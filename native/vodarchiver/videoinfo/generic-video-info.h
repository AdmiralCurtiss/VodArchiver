#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "i-video-info.h"

namespace VodArchiver {
struct GenericVideoInfo : public IVideoInfo {
    ~GenericVideoInfo() override;
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

    StreamService Service;
    std::string Username;
    std::string VideoId;
    std::string VideoTitle;
    std::string VideoGame;
    DateTime VideoTimestamp;
    TimeSpan VideoLength;
    RecordingState VideoRecordingState;
    VideoFileType VideoType;
};
} // namespace VodArchiver
