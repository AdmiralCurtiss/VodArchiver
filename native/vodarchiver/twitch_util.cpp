#include "twitch_util.h"

#include <chrono>
#include <format>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "rapidjson/document.h"

#include "util/number.h"
#include "util/text.h"

#include "vodarchiver/curl_util.h"
#include "vodarchiver/exec.h"
#include "vodarchiver/videoinfo/generic-video-info.h"
#include "vodarchiver/videoinfo/i-video-info.h"
#include "vodarchiver/videoinfo/youtube-video-info.h"

namespace VodArchiver::Twitch {
static constexpr char Helix[] = "https://api.twitch.tv/helix/";
static constexpr char OAuthUrl[] = "https://id.twitch.tv/oauth2/token";

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

static std::optional<std::string>
    Get(const std::string& url, const std::string& clientId, const std::string& clientSecret) {
    auto clientIdEscaped = VodArchiver::curl::UrlEscape(clientId);
    if (!clientIdEscaped) {
        return std::nullopt;
    }
    auto clientSecretEscaped = VodArchiver::curl::UrlEscape(clientSecret);
    if (!clientSecretEscaped) {
        return std::nullopt;
    }

    std::string formUrlEncodedContent = std::format("client_id={}&client_secret={}&grant_type={}",
                                                    *clientIdEscaped,
                                                    *clientSecretEscaped,
                                                    "client_credentials");
    auto tokenResponse =
        VodArchiver::curl::PostFormFromUrlToMemory(OAuthUrl, formUrlEncodedContent);
    if (!tokenResponse) {
        return std::nullopt;
    }

    std::string token;
    {
        rapidjson::Document json;
        json.Parse<rapidjson::kParseFullPrecisionFlag | rapidjson::kParseNanAndInfFlag
                       | rapidjson::kParseCommentsFlag,
                   rapidjson::UTF8<char>>(tokenResponse->data(), tokenResponse->size());
        if (json.HasParseError() || !json.IsObject()) {
            return std::nullopt;
        }
        const auto jo = json.GetObject();
        const auto tokenIt = jo.FindMember("access_token");
        if (tokenIt == jo.MemberEnd() || !tokenIt->value.IsString()) {
            return std::nullopt;
        }
        token = std::string(tokenIt->value.GetString(), tokenIt->value.GetStringLength());
    }


    std::vector<std::string> headers;
    headers.push_back(std::format("Authorization: Bearer {}", token));
    headers.push_back(std::format("Client-ID: {}", clientId));
    auto result = VodArchiver::curl::GetFromUrlToMemory(url, headers);
    if (!result) {
        return std::nullopt;
    }
    return std::string(result->data(), result->size());
}

std::optional<int64_t> GetUserIdFromUsername(const std::string& username,
                                             const std::string& clientId,
                                             const std::string& clientSecret) {
    auto usernameEscaped = VodArchiver::curl::UrlEscape(username);
    if (!usernameEscaped) {
        return std::nullopt;
    }
    std::string url = std::format("{}users?login={}", Helix, *usernameEscaped);
    auto result = Get(url, clientId, clientSecret);
    if (!result) {
        return std::nullopt;
    }

    rapidjson::Document json;
    json.Parse<rapidjson::kParseFullPrecisionFlag | rapidjson::kParseNanAndInfFlag
                   | rapidjson::kParseCommentsFlag,
               rapidjson::UTF8<char>>(result->data(), result->size());
    if (json.HasParseError() || !json.IsObject()) {
        return std::nullopt;
    }
    const auto jo = json.GetObject();
    const auto dataIt = jo.FindMember("data");
    if (dataIt == jo.MemberEnd() || !dataIt->value.IsArray()) {
        return std::nullopt;
    }
    auto array = dataIt->value.GetArray();
    if (array.Size() <= 0) {
        return std::nullopt;
    }
    for (auto& a : array) {
        if (!a.IsObject()) {
            return std::nullopt;
        }
        auto o = a.GetObject();
        auto idIt = o.FindMember("id");
        if (idIt == o.MemberEnd()) {
            return std::nullopt;
        }
        if (idIt->value.IsNumber()) {
            if (idIt->value.IsInt64()) {
                return idIt->value.GetInt64();
            }
            if (idIt->value.IsInt()) {
                return idIt->value.GetInt();
            }
            if (idIt->value.IsUint()) {
                return idIt->value.GetUint();
            }
            if (idIt->value.IsUint64()) {
                uint64_t u = idIt->value.GetUint64();
                if (u < static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
                    return static_cast<int64_t>(u);
                }
            }
        }
        if (idIt->value.IsString()) {
            std::string_view value(idIt->value.GetString(), idIt->value.GetStringLength());
            auto v = HyoutaUtils::NumberUtils::ParseInt64(value);
            if (v) {
                return *v;
            }
        }
        break;
    }
    return std::nullopt;
}

static std::optional<DateTime> ParseTwitchDateTime(std::string_view str) {
    std::string datestring(str);
    std::istringstream stream(datestring);
    stream.imbue(std::locale::classic());
    std::chrono::sys_time<std::chrono::seconds> time;
    stream >> std::chrono::parse("%Y-%m-%dT%H:%M:%SZ", time);
    if (!stream.fail()) {
        return DateTime::FromUnixTime(time.time_since_epoch().count());
    }
    return std::nullopt;
}

static std::optional<int64_t> ParseTwitchDuration(std::string_view v) {
    int64_t duration = 0;
    std::string sb;
    for (char c : v) {
        if (c >= '0' && c <= '9') {
            sb.push_back(c);
        } else {
            auto d = HyoutaUtils::NumberUtils::ParseInt64(sb);
            if (!d) {
                return std::nullopt;
            }
            switch (c) {
                case 'd': duration += (*d * static_cast<int64_t>(60 * 60 * 24)); break;
                case 'h': duration += (*d * static_cast<int64_t>(60 * 60)); break;
                case 'm': duration += (*d * static_cast<int64_t>(60)); break;
                case 's': duration += (*d); break;
                default: return std::nullopt;
            }
            sb.clear();
        }
    }
    return duration;
}

static std::optional<TwitchVideo>
    VideoFromJson(const rapidjson::GenericObject<true, rapidjson::Value>& json) {
    TwitchVideo v;
    {
        auto idString = ReadString(json, "id");
        if (!idString) {
            return std::nullopt;
        }
        std::string_view sv = *idString;
        if (sv.starts_with("v")) {
            sv = sv.substr(1);
        }
        auto id = HyoutaUtils::NumberUtils::ParseInt64(sv);
        if (!id) {
            return std::nullopt;
        }
        v.ID = *id;
    }
    {
        auto idString = ReadString(json, "user_id");
        if (!idString) {
            return std::nullopt;
        }
        auto id = HyoutaUtils::NumberUtils::ParseInt64(*idString);
        if (!id) {
            return std::nullopt;
        }
        v.UserID = *id;
    }
    {
        auto str = ReadString(json, "user_name");
        if (!str) {
            return std::nullopt;
        }
        v.Username = std::move(*str);
    }
    {
        auto str = ReadString(json, "title");
        if (str) {
            v.Title = std::move(*str);
        }
    }
    v.Game = "?";
    {
        auto str = ReadString(json, "description");
        if (str) {
            v.Description = std::move(*str);
        }
    }
    {
        auto str = ReadString(json, "created_at");
        if (str) {
            auto dt = ParseTwitchDateTime(*str);
            if (dt) {
                v.CreatedAt = *dt;
            }
        }
    }
    {
        auto str = ReadString(json, "published_at");
        if (str) {
            auto dt = ParseTwitchDateTime(*str);
            if (dt) {
                v.PublishedAt = *dt;
            }
        }
    }
    {
        auto str = ReadString(json, "duration");
        if (str) {
            auto dt = ParseTwitchDuration(*str);
            if (dt) {
                v.Duration = *dt;
            }
        }
    }
    {
        auto str = ReadString(json, "view_count");
        if (str) {
            auto id = HyoutaUtils::NumberUtils::ParseInt64(*str);
            if (id) {
                v.ViewCount = *id;
            }
        }
    }
    {
        v.Type = TwitchVideoType::Unknown;
        auto str = ReadString(json, "type");
        if (str) {
            if (*str == "archive") {
                v.Type = TwitchVideoType::Archive;
            } else if (*str == "highlight") {
                v.Type = TwitchVideoType::Highlight;
            } else if (*str == "upload") {
                v.Type = TwitchVideoType::Upload;
            }
        }
    }
    v.State = RecordingState::Unknown;
    return v;
}

static bool GetVideosInternal(TwitchVodFetchResult& r,
                              int64_t channelId,
                              bool highlights,
                              int offset,
                              int limit,
                              const std::string& clientId,
                              const std::string& clientSecret,
                              const std::string& after) {
    std::string url = std::format("{}videos?user_id={}&first={}&type={}{}{}",
                                  Helix,
                                  channelId,
                                  100,
                                  highlights ? "highlight" : "archive",
                                  after.empty() ? "" : "&after=",
                                  after);
    auto result = Get(url, clientId, clientSecret);
    if (!result) {
        return false;
    }

    std::string cursor;
    {
        rapidjson::Document json;
        json.Parse<rapidjson::kParseFullPrecisionFlag | rapidjson::kParseNanAndInfFlag
                       | rapidjson::kParseCommentsFlag,
                   rapidjson::UTF8<char>>(result->data(), result->size());
        if (json.HasParseError() || !json.IsObject()) {
            return false;
        }
        const auto jo = json.GetObject();
        const auto dataIt = jo.FindMember("data");
        if (dataIt == jo.MemberEnd() || !dataIt->value.IsArray()) {
            return false;
        }
        const auto array = dataIt->value.GetArray();
        for (const auto& a : array) {
            if (a.IsObject()) {
                const auto o = a.GetObject();
                auto video = VideoFromJson(o);
                if (video) {
                    r.Videos.push_back(std::move(*video));
                }
            }
        }

        const auto paginationIt = jo.FindMember("pagination");
        if (paginationIt != jo.MemberEnd() && paginationIt->value.IsObject()) {
            auto p = paginationIt->value.GetObject();
            auto cursorIt = p.FindMember("cursor");
            if (cursorIt != p.MemberEnd() && cursorIt->value.IsString()) {
                cursor =
                    std::string(cursorIt->value.GetString(), cursorIt->value.GetStringLength());
            }
        }
    }

    if (!cursor.empty()) {
        GetVideosInternal(r, channelId, highlights, offset, limit, clientId, clientSecret, cursor);
    }
}

std::optional<TwitchVodFetchResult> GetVideos(int64_t channelId,
                                              bool highlights,
                                              int offset,
                                              int limit,
                                              const std::string& clientId,
                                              const std::string& clientSecret) {
    TwitchVodFetchResult r;
    GetVideosInternal(r, channelId, highlights, offset, limit, clientId, clientSecret, "");
    r.TotalVideoCount = 1; // TODO
    return r;
}
} // namespace VodArchiver::Twitch
