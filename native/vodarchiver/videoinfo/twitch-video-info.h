#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "../time_types.h"

#include "i-video-info.h"

namespace VodArchiver {
enum class TwitchVideoType { Upload, Archive, Highlight, Unknown };
std::string_view TwitchVideoTypeToString(TwitchVideoType type);
std::optional<TwitchVideoType> TwitchVideoTypeFromString(std::string_view sv);

struct TwitchVideo {
    int64_t ID = 0;
    int64_t UserID = 0;
    std::optional<std::string> Username;
    std::optional<std::string> Title;
    std::optional<std::string> Game;
    std::optional<std::string> Description;
    std::optional<DateTime> CreatedAt;
    std::optional<DateTime> PublishedAt;
    int64_t Duration = 0;
    int64_t ViewCount = 0;
    TwitchVideoType Type = TwitchVideoType::Archive;
    RecordingState State = RecordingState::Unknown;
};

struct TwitchVideoInfo : public IVideoInfo {
    ~TwitchVideoInfo() override;
    StreamService GetService() const override;
    std::string GetUsername() const override;
    std::string GetVideoId() const override;
    std::string GetVideoTitle() const override;
    std::string GetVideoGame() const override;
    DateTime GetVideoTimestamp() const override;
    TimeSpan GetVideoLength() const override;
    RecordingState GetVideoRecordingState() const override;
    VideoFileType GetVideoType() const override;

    std::unique_ptr<IVideoInfo> Clone() const override;

    TwitchVideo _Video;
    StreamService _Service;
};
} // namespace VodArchiver
