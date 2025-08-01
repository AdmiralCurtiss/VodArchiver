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

std::string_view GenericVideoInfo::GetUsername(std::array<char, 256>& buffer) const {
    return this->Username;
}

std::string_view GenericVideoInfo::GetVideoId(std::array<char, 256>& buffer) const {
    return this->VideoId;
}

std::string_view GenericVideoInfo::GetVideoTitle(std::array<char, 256>& buffer) const {
    return this->VideoTitle;
}

std::string_view GenericVideoInfo::GetVideoGame(std::array<char, 256>& buffer) const {
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
