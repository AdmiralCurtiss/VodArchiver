#include "generic-video-info.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace VodArchiver {
GenericVideoInfo::~GenericVideoInfo() = default;

StreamService GenericVideoInfo::GetService() const {
    return this->Service;
}

std::string GenericVideoInfo::GetUsername() const {
    return this->Username;
}

std::string GenericVideoInfo::GetVideoId() const {
    return this->VideoId;
}

std::string GenericVideoInfo::GetVideoTitle() const {
    return this->VideoTitle;
}

std::string GenericVideoInfo::GetVideoGame() const {
    return this->VideoGame;
}

DateTime GenericVideoInfo::GetVideoTimestamp() const {
    return this->VideoTimestamp;
}

TimeSpan GenericVideoInfo::GetVideoLength() const {
    return this->VideoLength;
}

RecordingState GenericVideoInfo::GetVideoRecordingState() const {
    return this->VideoRecordingState;
}

VideoFileType GenericVideoInfo::GetVideoType() const {
    return this->VideoType;
}

std::unique_ptr<IVideoInfo> GenericVideoInfo::Clone() const {
    return std::make_unique<GenericVideoInfo>(*this);
}
} // namespace VodArchiver
