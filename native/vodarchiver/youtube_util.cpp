#include "youtube_util.h"

#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "rapidjson/document.h"

#include "util/number.h"
#include "util/text.h"

#include "vodarchiver/exec.h"
#include "vodarchiver/videoinfo/generic-video-info.h"
#include "vodarchiver/videoinfo/i-video-info.h"
#include "vodarchiver/videoinfo/youtube-video-info.h"

namespace VodArchiver::Youtube {
static std::optional<std::string>
    ReadString(const rapidjson::GenericObject<true, rapidjson::Value>& json, const char* key) {
    auto it = json.FindMember(key);
    if (it == json.MemberEnd()) {
        return std::nullopt;
    }
    auto& j = it->value;
    if (j.IsString()) {
        const char* str = j.GetString();
        const auto len = j.GetStringLength();
        return std::string(str, len);
    }
    return std::nullopt;
}

static std::optional<double>
    ReadDouble(const rapidjson::GenericObject<true, rapidjson::Value>& json, const char* key) {
    auto it = json.FindMember(key);
    if (it == json.MemberEnd()) {
        return std::nullopt;
    }
    auto& j = it->value;
    if (j.IsNumber()) {
        const auto i = j.GetDouble();
        return static_cast<double>(i);
    }
    return std::nullopt;
}

static RetrieveVideoResultStruct
    ParseFromJsonFull(const rapidjson::GenericObject<true, rapidjson::Value>& json,
                      std::string_view usernameIfNotInJson) {
    auto y = std::make_unique<YoutubeVideoInfo>();
    y->Username = ReadString(json, "uploader_id").value_or("");
    if (HyoutaUtils::TextUtils::Trim(y->Username).empty()) {
        y->Username = std::string(usernameIfNotInJson);
        return RetrieveVideoResultStruct{.result = RetrieveVideoResult::ParseFailure,
                                         .info = std::move(y)};
    }
    auto videoId = ReadString(json, "id");
    if (!videoId) {
        return RetrieveVideoResultStruct{.result = RetrieveVideoResult::ParseFailure,
                                         .info = std::move(y)};
    }
    y->VideoId = std::move(*videoId);
    auto videoTitle = ReadString(json, "title");
    if (!videoTitle) {
        return RetrieveVideoResultStruct{.result = RetrieveVideoResult::ParseFailure,
                                         .info = std::move(y)};
    }
    y->VideoTitle = std::move(*videoTitle);

    y->VideoGame = "";
    auto tagsIt = json.FindMember("tags");
    if (tagsIt != json.MemberEnd() && tagsIt->value.IsArray()) {
        std::string sb;
        for (const auto& tag : tagsIt->value.GetArray()) {
            if (tag.IsString()) {
                sb.append(tag.GetString());
                sb.append("; ");
            } else {
                return RetrieveVideoResultStruct{.result = RetrieveVideoResult::ParseFailure,
                                                 .info = std::move(y)};
            }
        }

        if (sb.size() >= 2) {
            sb.pop_back();
            sb.pop_back();
            y->VideoGame = std::move(sb);
        }
    }

    auto datetimestring = ReadString(json, "upload_date");
    if (!datetimestring || HyoutaUtils::TextUtils::Trim(*datetimestring).empty()) {
        return RetrieveVideoResultStruct{.result = RetrieveVideoResult::ParseFailure,
                                         .info = std::move(y)};
    }
    auto datetimeuint = HyoutaUtils::NumberUtils::ParseUInt64(*datetimestring);
    if (!datetimeuint) {
        return RetrieveVideoResultStruct{.result = RetrieveVideoResult::ParseFailure,
                                         .info = std::move(y)};
    }
    y->VideoTimestamp = DateTime::FromDate(
        *datetimeuint / 10000, (*datetimeuint / 100) % 100, (*datetimeuint) % 100, 0, 0, 0);

    auto durationdouble = ReadDouble(json, "duration");
    if (!durationdouble) {
        return RetrieveVideoResultStruct{.result = RetrieveVideoResult::ParseFailure,
                                         .info = std::move(y)};
    }

    y->VideoLength = TimeSpan::FromSeconds(*durationdouble);
    y->VideoRecordingState = RecordingState::Recorded;
    y->VideoType = VideoFileType::Unknown;

    auto uploader = ReadString(json, "uploader");
    if (!uploader) {
        return RetrieveVideoResultStruct{.result = RetrieveVideoResult::ParseFailure,
                                         .info = std::move(y)};
    }
    y->UserDisplayName = std::move(*uploader);
    auto description = ReadString(json, "description");
    if (!description) {
        return RetrieveVideoResultStruct{.result = RetrieveVideoResult::ParseFailure,
                                         .info = std::move(y)};
    }
    y->VideoDescription = std::move(*description);
    return RetrieveVideoResultStruct{.result = RetrieveVideoResult::Success, .info = std::move(y)};
}

static RetrieveVideoResultStruct
    ParseFromJsonFlat(const rapidjson::GenericObject<true, rapidjson::Value>& json,
                      std::string_view usernameIfNotInJson) {
    auto y = std::make_unique<GenericVideoInfo>();
    y->Username = std::string(usernameIfNotInJson);

    auto videoId = ReadString(json, "id");
    if (!videoId) {
        return RetrieveVideoResultStruct{.result = RetrieveVideoResult::ParseFailure,
                                         .info = std::move(y)};
    }
    y->VideoId = std::move(*videoId);
    auto videoTitle = ReadString(json, "title");
    if (!videoTitle) {
        return RetrieveVideoResultStruct{.result = RetrieveVideoResult::ParseFailure,
                                         .info = std::move(y)};
    }
    y->VideoTitle = std::move(*videoTitle);

    y->Service = StreamService::Youtube;
    y->VideoTimestamp = DateTime::UtcNow();
    return RetrieveVideoResultStruct{.result = RetrieveVideoResult::Success, .info = std::move(y)};
}

static RetrieveVideoResultStruct
    ParseFromJson(const rapidjson::GenericObject<true, rapidjson::Value>& json,
                  bool flat,
                  std::string_view usernameIfNotInJson) {
    if (flat) {
        return ParseFromJsonFlat(json, usernameIfNotInJson);
    } else {
        return ParseFromJsonFull(json, usernameIfNotInJson);
    }
}

RetrieveVideoResultStruct
    RetrieveVideo(std::string_view id, std::string_view usernameIfNotInJson, bool wantCookies) {
    std::string raw;
    {
        std::vector<std::string> args;
        args.push_back("-j");
        args.push_back(std::format("https://www.youtube.com/watch?v={}", id));
        if (wantCookies) {
            args.push_back("--cookies");
            args.push_back("d:\\cookies.txt");
        }
        RunProgram(
            "yt-dlp.exe",
            args,
            [&](std::string_view a) { raw.append(a); },
            [&](std::string_view) {});
    }

    rapidjson::Document json;
    json.Parse<rapidjson::kParseFullPrecisionFlag | rapidjson::kParseNanAndInfFlag
                   | rapidjson::kParseCommentsFlag,
               rapidjson::UTF8<char>>(raw.data(), raw.size());
    if (json.HasParseError() || !json.IsObject()) {
        return RetrieveVideoResultStruct{.result = RetrieveVideoResult::FetchFailure,
                                         .info = nullptr};
    }

    const rapidjson::Document& constJson = json;
    return ParseFromJson(constJson.GetObject(), false, usernameIfNotInJson);
}

static std::optional<std::vector<std::unique_ptr<IVideoInfo>>>
    RetrieveVideosFromParameterString(const std::string& parameter,
                                      bool flat,
                                      const std::string& usernameIfNotInJson) {
    std::string raw;
    {
        std::vector<std::string> args;
        if (flat) {
            args.push_back("--flat-playlist");
        }
        args.push_back("--ignore-errors");
        args.push_back("-J");
        args.push_back(parameter);
        RunProgram(
            "yt-dlp.exe",
            args,
            [&](std::string_view a) { raw.append(a); },
            [&](std::string_view) {});
    }
    // try parsing regardless of exit code
    rapidjson::Document json;
    json.Parse<rapidjson::kParseFullPrecisionFlag | rapidjson::kParseNanAndInfFlag
                   | rapidjson::kParseCommentsFlag,
               rapidjson::UTF8<char>>(raw.data(), raw.size());
    if (json.HasParseError() || !json.IsObject()) {
        return std::nullopt;
    }

    const auto jo = json.GetObject();
    const auto entriesIt = jo.FindMember("entries");
    if (entriesIt == jo.MemberEnd() || !entriesIt->value.IsArray()) {
        return std::nullopt;
    }
    const auto jsonEntries = entriesIt->value.GetArray();

    std::vector<std::unique_ptr<IVideoInfo>> list;
    for (const auto& jsonEntry : jsonEntries) {
        if (jsonEntry.IsObject()) {
            auto d = ParseFromJson(jsonEntry.GetObject(), flat, usernameIfNotInJson);
            if (d.result == RetrieveVideoResult::Success) {
                list.push_back(std::move(d.info));
            }
        }
    }
    return list;
}

std::optional<std::vector<std::unique_ptr<IVideoInfo>>>
    RetrieveVideosFromPlaylist(const std::string& playlist,
                               bool flat,
                               const std::string& usernameIfNotInJson) {
    return RetrieveVideosFromParameterString(
        "https://www.youtube.com/playlist?list=" + playlist, flat, usernameIfNotInJson);
}

std::optional<std::vector<std::unique_ptr<IVideoInfo>>>
    RetrieveVideosFromChannel(const std::string& channel,
                              bool flat,
                              const std::string& usernameIfNotInJson) {
    return RetrieveVideosFromParameterString(
        "https://www.youtube.com/channel/" + channel, flat, usernameIfNotInJson);
}

std::optional<std::vector<std::unique_ptr<IVideoInfo>>>
    RetrieveVideosFromUser(const std::string& user, bool flat) {
    return RetrieveVideosFromParameterString("ytuser:" + user, flat, user);
}

std::optional<std::vector<std::unique_ptr<IVideoInfo>>>
    RetrieveVideosFromUrl(const std::string& url, bool flat) {
    return RetrieveVideosFromParameterString(url, flat, url);
}
} // namespace VodArchiver::Youtube
