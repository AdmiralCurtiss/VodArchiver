#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "i-video-info.h"

namespace VodArchiver {
struct YoutubeVideoInfo : public IVideoInfo {
    ~YoutubeVideoInfo() override;
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

    std::unique_ptr<IVideoInfo> Clone() const override;

    std::string Username;
    std::string VideoId;
    std::string VideoTitle;
    std::string VideoGame;
    DateTime VideoTimestamp;
    TimeSpan VideoLength;
    RecordingState VideoRecordingState;
    VideoFileType VideoType;
    std::string UserDisplayName;
    std::string VideoDescription;
};
} // namespace VodArchiver
