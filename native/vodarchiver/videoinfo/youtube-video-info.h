#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "i-video-info.h"

namespace VodArchiver {
struct YoutubeVideoInfo : public IVideoInfo {
    ~YoutubeVideoInfo() override;
    StreamService GetService() const override;
    std::string_view GetUsername(std::array<char, 256>& buffer) const override;
    std::string_view GetVideoId(std::array<char, 256>& buffer) const override;
    std::string_view GetVideoTitle(std::array<char, 256>& buffer) const override;
    std::string_view GetVideoGame(std::array<char, 256>& buffer) const override;
    DateTime GetVideoTimestamp() const override;
    TimeSpan GetVideoLength() const override;
    RecordingState GetVideoRecordingState() const override;
    VideoFileType GetVideoType() const override;

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
