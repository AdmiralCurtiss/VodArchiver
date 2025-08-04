#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "../time_types.h"

namespace VodArchiver {
enum class StreamService : uint8_t {
    Unknown = 0,
    Twitch,
    Hitbox,
    TwitchChatReplay,
    Youtube,
    RawUrl,
    FFMpegJob,
    COUNT
};
std::string_view StreamServiceToString(StreamService service);
std::optional<StreamService> StreamServiceFromString(std::string_view sv);

enum class RecordingState : uint8_t {
    Unknown,
    Live,
    Recorded,
};
std::string_view RecordingStateToString(RecordingState state);
std::optional<RecordingState> RecordingStateFromString(std::string_view sv);

enum class VideoFileType : uint8_t { M3U, FLVs, Unknown };
std::string_view VideoFileTypeToString(VideoFileType type);
std::optional<VideoFileType> VideoFileTypeFromString(std::string_view sv);

struct IVideoInfo {
    // To explain this weird design here...
    // We want querying to work without allocating memory. However, not all VideoInfos store the
    // string-like information as actual strings internally. So we let the user pass a buffer that
    // may or may not be used, then return a string_view that either points to the internal string
    // or the given buffer.
    // The string_views returned here are guaranteed to be null terminated.

    virtual ~IVideoInfo();
    virtual StreamService GetService() const = 0;
    virtual std::string_view GetUsername(std::array<char, 256>& buffer) const = 0;
    virtual std::string_view GetVideoId(std::array<char, 256>& buffer) const = 0;
    virtual std::string_view GetVideoTitle(std::array<char, 256>& buffer) const = 0;
    virtual std::string_view GetVideoGame(std::array<char, 256>& buffer) const = 0;
    virtual DateTime GetVideoTimestamp() const = 0;
    virtual TimeSpan GetVideoLength() const = 0;
    virtual RecordingState GetVideoRecordingState() const = 0;
    virtual VideoFileType GetVideoType() const = 0;

    virtual std::unique_ptr<IVideoInfo> Clone() const = 0;
};
} // namespace VodArchiver
