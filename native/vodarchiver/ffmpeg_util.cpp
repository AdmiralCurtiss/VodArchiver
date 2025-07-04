#include "ffmpeg_util.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rapidjson/document.h"

#include "exec.h"
#include "time_types.h"
#include "util/file.h"
#include "util/number.h"
#include "util/text.h"

namespace VodArchiver {
static float ParseFramerate(std::string_view v) {
    auto arr = HyoutaUtils::TextUtils::Split(v, "/");
    if (arr.size() >= 2) {
        if (auto a0 = HyoutaUtils::NumberUtils::ParseInt64(arr[0])) {
            if (auto a1 = HyoutaUtils::NumberUtils::ParseInt64(arr[1])) {
                return (float)(((double)*a0) / ((double)*a1));
            }
        }
    }
    return 0.0f;
}

template<typename T>
static std::optional<uint32_t> ReadUInt32(T& json, const char* key) {
    auto it = json.FindMember(key);
    if (it == json.MemberEnd()) {
        return std::nullopt;
    }
    auto& j = it->value;
    if (j.IsInt64()) {
        const auto i = j.GetInt64();
        if (i < 0) {
            return std::nullopt;
        }
        if (i > std::numeric_limits<uint32_t>::max()) {
            return std::nullopt;
        }
        return static_cast<uint32_t>(i);
    }
    return std::nullopt;
}

template<typename T>
static std::optional<uint64_t> ReadUInt64(T& json, const char* key) {
    auto it = json.FindMember(key);
    if (it == json.MemberEnd()) {
        return std::nullopt;
    }
    auto& j = it->value;
    if (j.IsInt64()) {
        const auto i = j.GetInt64();
        if (i < 0) {
            return std::nullopt;
        }
        return static_cast<uint64_t>(i);
    }
    return std::nullopt;
}

template<typename T>
static std::optional<double> ReadDouble(T& json, const char* key) {
    auto it = json.FindMember(key);
    if (it == json.MemberEnd()) {
        return std::nullopt;
    }
    auto& j = it->value;
    if (j.IsDouble()) {
        const auto i = j.GetDouble();
        return static_cast<double>(i);
    }
    return std::nullopt;
}

template<typename T>
static std::optional<std::string> ReadString(T& json, const char* key) {
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

std::optional<FFProbeResult> FFMpegProbe(const std::string& filename) {
    std::string out;
    int retval = RunProgram(
        "ffprobe.exe",
        {{"-show_format", "-show_streams", "-print_format", "json", filename}},
        [&](std::string_view sv) { out.append(sv); },
        [&](std::string_view) {});

    rapidjson::Document json;
    json.Parse<rapidjson::kParseFullPrecisionFlag | rapidjson::kParseNanAndInfFlag
                   | rapidjson::kParseCommentsFlag,
               rapidjson::UTF8<char>>(out.data(), out.size());
    if (json.HasParseError() || !json.IsObject()) {
        return std::nullopt;
    }

    const auto jo = json.GetObject();
    const auto formatIt = jo.FindMember("format");
    if (formatIt == jo.MemberEnd() || !formatIt->value.IsObject()) {
        return std::nullopt;
    }
    const auto jsonFormat = formatIt->value.GetObject();

    std::string path = HyoutaUtils::IO::GetAbsolutePath(std::string_view(filename));
    DateTime timestamp;
    // TODO
    // timestamp = System.IO.File.GetCreationTimeUtc(filename);
    auto filesize =
        HyoutaUtils::NumberUtils::ParseUInt64(ReadString(jsonFormat, "size").value_or(""));
    if (!filesize.has_value()) {
        return std::nullopt;
    }
    auto bitrate =
        HyoutaUtils::NumberUtils::ParseUInt64(ReadString(jsonFormat, "bit_rate").value_or(""));
    if (!bitrate.has_value()) {
        return std::nullopt;
    }
    auto durationDouble =
        HyoutaUtils::NumberUtils::ParseDouble(ReadString(jsonFormat, "duration").value_or(""));
    if (!durationDouble.has_value()) {
        return std::nullopt;
    }
    TimeSpan duration = TimeSpan::FromSeconds(*durationDouble);
    std::vector<FFProbeStream> streams;

    const auto streamsIt = jo.FindMember("streams");
    if (streamsIt == jo.MemberEnd() || !streamsIt->value.IsArray()) {
        return std::nullopt;
    }
    const auto jsonStreams = streamsIt->value.GetArray();

    for (const auto& jsonStream : jsonStreams) {
        auto index = ReadUInt32(jsonStream, "index");
        if (!index.has_value()) {
            continue;
        }
        auto codecTag =
            HyoutaUtils::NumberUtils::ParseUInt32(ReadString(jsonStream, "codec_tag").value_or(""));
        if (!codecTag.has_value()) {
            continue;
        }
        auto codecType = ReadString(jsonStream, "codec_type");
        if (!codecType.has_value()) {
            continue;
        }
        auto duration =
            HyoutaUtils::NumberUtils::ParseDouble(ReadString(jsonStream, "duration").value_or(""));
        auto framerate = ReadString(jsonStream, "r_frame_rate");

        streams.emplace_back(FFProbeStream{
            .Index = *index,
            .CodecTag = *codecTag,
            .CodecType = std::move(*codecType),
            .Duration = duration.has_value() ? std::make_optional(TimeSpan::FromSeconds(*duration))
                                             : std::nullopt,
            .Framerate = framerate.has_value() ? ParseFramerate(*framerate) : 0.0f});
    }

    return FFProbeResult{
        .Path = std::move(path),
        .Timestamp = timestamp,
        .Filesize = *filesize,
        .Bitrate = *bitrate,
        .Duration = duration,
        .Streams = std::move(streams),
    };
}
} // namespace VodArchiver
