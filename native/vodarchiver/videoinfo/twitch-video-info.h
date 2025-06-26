#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "../time_types.h"

#include "i-video-info.h"

namespace VodArchiver {
enum class TwitchVideoType { Upload, Archive, Highlight };
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
    void SetService(StreamService service) override;
    std::string GetUsername() const override;
    void SetUsername(std::string username) override;
    std::string GetVideoId() const override;
    void SetVideoId(std::string videoId) override;
    std::string GetVideoTitle() const override;
    void SetVideoTitle(std::string videoTitle) override;
    std::string GetVideoGame() const override;
    void SetVideoGame(std::string videoGame) override;
    DateTime GetVideoTimestamp() const override;
    void SetVideoTimestamp(DateTime timestamp) override;
    TimeSpan GetVideoLength() const override;
    void SetVideoLength(TimeSpan length) override;
    RecordingState GetVideoRecordingState() const override;
    void SetVideoRecordingState(RecordingState videoRecordingState) override;
    VideoFileType GetVideoType() const override;
    void SetVideoType(VideoFileType videoType) override;

    TwitchVideo _Video;
    StreamService _Service;
};
} // namespace VodArchiver
