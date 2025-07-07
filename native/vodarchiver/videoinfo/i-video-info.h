#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "../time_types.h"

namespace VodArchiver {
enum class StreamService {
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

enum class RecordingState {
    Unknown,
    Live,
    Recorded,
};
std::string_view RecordingStateToString(RecordingState state);
std::optional<RecordingState> RecordingStateFromString(std::string_view sv);

enum class VideoFileType { M3U, FLVs, Unknown };
std::string_view VideoFileTypeToString(VideoFileType type);
std::optional<VideoFileType> VideoFileTypeFromString(std::string_view sv);

struct IVideoInfo {
    virtual ~IVideoInfo();
    virtual StreamService GetService() const = 0;
    virtual void SetService(StreamService service) = 0;
    virtual std::string GetUsername() const = 0;
    virtual void SetUsername(std::string username) = 0;
    virtual std::string GetVideoId() const = 0;
    virtual void SetVideoId(std::string videoId) = 0;
    virtual std::string GetVideoTitle() const = 0;
    virtual void SetVideoTitle(std::string videoTitle) = 0;
    virtual std::string GetVideoGame() const = 0;
    virtual void SetVideoGame(std::string videoGame) = 0;
    virtual DateTime GetVideoTimestamp() const = 0;
    virtual void SetVideoTimestamp(DateTime timestamp) = 0;
    virtual TimeSpan GetVideoLength() const = 0;
    virtual void SetVideoLength(TimeSpan length) = 0;
    virtual RecordingState GetVideoRecordingState() const = 0;
    virtual void SetVideoRecordingState(RecordingState videoRecordingState) = 0;
    virtual VideoFileType GetVideoType() const = 0;
    virtual void SetVideoType(VideoFileType videoType) = 0;

    virtual std::unique_ptr<IVideoInfo> Clone() const = 0;
};
} // namespace VodArchiver
