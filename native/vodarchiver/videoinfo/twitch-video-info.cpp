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
        case TwitchVideoType::Unknown:
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
    } else if (sv == "Unknown") {
        return TwitchVideoType::Unknown;
    }
    return std::nullopt;
}

TwitchVideoInfo::~TwitchVideoInfo() = default;

StreamService TwitchVideoInfo::GetService() const {
    return this->_Service;
}

std::string TwitchVideoInfo::GetUsername() const {
    return this->_Video.Username.value_or("");
}

std::string TwitchVideoInfo::GetVideoGame() const {
    return this->_Video.Game.value_or("");
}

std::string TwitchVideoInfo::GetVideoId() const {
    return std::format("{}", _Video.ID);
}

TimeSpan TwitchVideoInfo::GetVideoLength() const {
    return TimeSpan::FromSeconds(this->_Video.Duration);
}

RecordingState TwitchVideoInfo::GetVideoRecordingState() const {
    return this->_Video.State;
}

DateTime TwitchVideoInfo::GetVideoTimestamp() const {
    return this->_Video.PublishedAt.value_or(DateTime::FromBinary(0));
}

std::string TwitchVideoInfo::GetVideoTitle() const {
    return this->_Video.Title.value_or("");
}

VideoFileType TwitchVideoInfo::GetVideoType() const {
    return VideoFileType::M3U;
}

std::unique_ptr<IVideoInfo> TwitchVideoInfo::Clone() const {
    return std::make_unique<TwitchVideoInfo>(*this);
}
} // namespace VodArchiver
