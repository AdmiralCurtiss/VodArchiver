#include "youtube-video-info.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace VodArchiver {
YoutubeVideoInfo::~YoutubeVideoInfo() = default;

StreamService YoutubeVideoInfo::GetService() const {
    return StreamService::Youtube;
}

std::string_view YoutubeVideoInfo::GetUsername(std::array<char, 256>& buffer) const {
    return this->Username;
}

std::string_view YoutubeVideoInfo::GetVideoId(std::array<char, 256>& buffer) const {
    return this->VideoId;
}

std::string_view YoutubeVideoInfo::GetVideoTitle(std::array<char, 256>& buffer) const {
    return this->VideoTitle;
}

std::string_view YoutubeVideoInfo::GetVideoGame(std::array<char, 256>& buffer) const {
    return this->VideoGame;
}

DateTime YoutubeVideoInfo::GetVideoTimestamp() const {
    return this->VideoTimestamp;
}

TimeSpan YoutubeVideoInfo::GetVideoLength() const {
    return this->VideoLength;
}

RecordingState YoutubeVideoInfo::GetVideoRecordingState() const {
    return this->VideoRecordingState;
}

VideoFileType YoutubeVideoInfo::GetVideoType() const {
    return this->VideoType;
}

std::unique_ptr<IVideoInfo> YoutubeVideoInfo::Clone() const {
    return std::make_unique<YoutubeVideoInfo>(*this);
}
} // namespace VodArchiver
