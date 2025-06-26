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

void YoutubeVideoInfo::SetService(StreamService service) {
    throw std::runtime_error("cannot set service for YoutubeVideoInfo");
}

std::string YoutubeVideoInfo::GetUsername() const {
    return this->Username;
}

void YoutubeVideoInfo::SetUsername(std::string username) {
    this->Username = std::move(username);
}

std::string YoutubeVideoInfo::GetVideoId() const {
    return this->VideoId;
}

void YoutubeVideoInfo::SetVideoId(std::string videoId) {
    this->VideoId = std::move(videoId);
}

std::string YoutubeVideoInfo::GetVideoTitle() const {
    return this->VideoTitle;
}

void YoutubeVideoInfo::SetVideoTitle(std::string videoTitle) {
    this->VideoTitle = std::move(videoTitle);
}

std::string YoutubeVideoInfo::GetVideoGame() const {
    return this->VideoGame;
}

void YoutubeVideoInfo::SetVideoGame(std::string videoGame) {
    this->VideoGame = std::move(videoGame);
}

DateTime YoutubeVideoInfo::GetVideoTimestamp() const {
    return this->VideoTimestamp;
}

void YoutubeVideoInfo::SetVideoTimestamp(DateTime timestamp) {
    this->VideoTimestamp = timestamp;
}

TimeSpan YoutubeVideoInfo::GetVideoLength() const {
    return this->VideoLength;
}

void YoutubeVideoInfo::SetVideoLength(TimeSpan length) {
    this->VideoLength = length;
}

RecordingState YoutubeVideoInfo::GetVideoRecordingState() const {
    return this->VideoRecordingState;
}

void YoutubeVideoInfo::SetVideoRecordingState(RecordingState videoRecordingState) {
    this->VideoRecordingState = videoRecordingState;
}

VideoFileType YoutubeVideoInfo::GetVideoType() const {
    return this->VideoType;
}

void YoutubeVideoInfo::SetVideoType(VideoFileType videoType) {
    this->VideoType = videoType;
}
} // namespace VodArchiver
