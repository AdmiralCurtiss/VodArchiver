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

void GenericVideoInfo::SetService(StreamService service) {
    this->Service = service;
}

std::string GenericVideoInfo::GetUsername() const {
    return this->Username;
}

void GenericVideoInfo::SetUsername(std::string username) {
    this->Username = std::move(username);
}

std::string GenericVideoInfo::GetVideoId() const {
    return this->VideoId;
}

void GenericVideoInfo::SetVideoId(std::string videoId) {
    this->VideoId = std::move(videoId);
}

std::string GenericVideoInfo::GetVideoTitle() const {
    return this->VideoTitle;
}

void GenericVideoInfo::SetVideoTitle(std::string videoTitle) {
    this->VideoTitle = std::move(videoTitle);
}

std::string GenericVideoInfo::GetVideoGame() const {
    return this->VideoGame;
}

void GenericVideoInfo::SetVideoGame(std::string videoGame) {
    this->VideoGame = std::move(videoGame);
}

DateTime GenericVideoInfo::GetVideoTimestamp() const {
    return this->VideoTimestamp;
}

void GenericVideoInfo::SetVideoTimestamp(DateTime timestamp) {
    this->VideoTimestamp = timestamp;
}

TimeSpan GenericVideoInfo::GetVideoLength() const {
    return this->VideoLength;
}

void GenericVideoInfo::SetVideoLength(TimeSpan length) {
    this->VideoLength = length;
}

RecordingState GenericVideoInfo::GetVideoRecordingState() const {
    return this->VideoRecordingState;
}

void GenericVideoInfo::SetVideoRecordingState(RecordingState videoRecordingState) {
    this->VideoRecordingState = videoRecordingState;
}

VideoFileType GenericVideoInfo::GetVideoType() const {
    return this->VideoType;
}

void GenericVideoInfo::SetVideoType(VideoFileType videoType) {
    this->VideoType = videoType;
}
} // namespace VodArchiver
