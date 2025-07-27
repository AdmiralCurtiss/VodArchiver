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

std::string YoutubeVideoInfo::GetUsername() const {
    return this->Username;
}

std::string YoutubeVideoInfo::GetVideoId() const {
    return this->VideoId;
}

std::string YoutubeVideoInfo::GetVideoTitle() const {
    return this->VideoTitle;
}

std::string YoutubeVideoInfo::GetVideoGame() const {
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
