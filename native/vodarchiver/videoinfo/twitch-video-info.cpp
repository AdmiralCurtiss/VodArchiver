#include "twitch-video-info.h"

#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "util/number.h"

namespace VodArchiver {
std::string_view TwitchVideoTypeToString(TwitchVideoType type) {
    switch (type) {
        case TwitchVideoType::Upload: return "Upload";
        case TwitchVideoType::Archive: return "Archive";
        case TwitchVideoType::Highlight: return "Highlight";
        default: return "Unknown";
    }
}

std::optional<TwitchVideoType> TwitchVideoTypeFromString(std::string_view sv) {
    if (sv == "Upload") {
        return TwitchVideoType::Upload;
    } else if (sv == "Archive") {
        return TwitchVideoType::Archive;
    } else if (sv == "Highlight") {
        return TwitchVideoType::Highlight;
    }
    return std::nullopt;
}

TwitchVideoInfo::~TwitchVideoInfo() = default;

StreamService TwitchVideoInfo::GetService() const {
    return this->_Service;
}

void TwitchVideoInfo::SetService(StreamService service) {
    this->_Service = service;
}

std::string TwitchVideoInfo::GetUsername() const {
    return this->_Video.Username.value_or("");
}

void TwitchVideoInfo::SetUsername(std::string username) {
    this->_Video.Username = std::move(username);
}

std::string TwitchVideoInfo::GetVideoGame() const {
    return this->_Video.Game.value_or("");
}

void TwitchVideoInfo::SetVideoGame(std::string videoGame) {
    this->_Video.Game = std::move(videoGame);
}

std::string TwitchVideoInfo::GetVideoId() const {
    return std::format("{}", _Video.ID);
}

void TwitchVideoInfo::SetVideoId(std::string videoId) {
    auto v = HyoutaUtils::NumberUtils::ParseInt64(videoId);
    if (!v) {
        throw std::runtime_error(std::format("failed to parse video ID '{}'", videoId));
    }
    this->_Video.ID = *v;
}

TimeSpan TwitchVideoInfo::GetVideoLength() const {
    return TimeSpan::FromSeconds(this->_Video.Duration);
}

void TwitchVideoInfo::SetVideoLength(TimeSpan length) {
    this->_Video.Duration = static_cast<int64_t>(length.GetTotalSeconds());
}

RecordingState TwitchVideoInfo::GetVideoRecordingState() const {
    return this->_Video.State;
}

void TwitchVideoInfo::SetVideoRecordingState(RecordingState videoRecordingState) {
    this->_Video.State = videoRecordingState;
}

DateTime TwitchVideoInfo::GetVideoTimestamp() const {
    return this->_Video.PublishedAt.value_or(DateTime::FromBinary(0));
}

void TwitchVideoInfo::SetVideoTimestamp(DateTime timestamp) {
    this->_Video.PublishedAt = timestamp;
}

std::string TwitchVideoInfo::GetVideoTitle() const {
    return this->_Video.Title.value_or("");
}

void TwitchVideoInfo::SetVideoTitle(std::string videoTitle) {
    this->_Video.Title = std::move(videoTitle);
}

VideoFileType TwitchVideoInfo::GetVideoType() const {
    return VideoFileType::M3U;
}

void TwitchVideoInfo::SetVideoType(VideoFileType videoType) {
    throw std::runtime_error("cannot set video type for TwitchVideoInfo");
}
} // namespace VodArchiver
