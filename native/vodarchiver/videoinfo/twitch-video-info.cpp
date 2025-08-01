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
    return this->Service;
}

std::string_view TwitchVideoInfo::GetUsername(std::array<char, 256>& buffer) const {
    if (this->Video.Username.has_value()) {
        return *this->Video.Username;
    }
    buffer[0] = '\0';
    return std::string_view(buffer.data(), 0);
}

std::string_view TwitchVideoInfo::GetVideoGame(std::array<char, 256>& buffer) const {
    if (this->Video.Game.has_value()) {
        return *this->Video.Game;
    }
    buffer[0] = '\0';
    return std::string_view(buffer.data(), 0);
}

std::string_view TwitchVideoInfo::GetVideoId(std::array<char, 256>& buffer) const {
    auto result = std::format_to_n(buffer.data(), buffer.size() - 1, "{}", this->Video.ID);
    *result.out = '\0';
    return std::string_view(buffer.data(), result.out);
}

TimeSpan TwitchVideoInfo::GetVideoLength() const {
    return TimeSpan::FromSeconds(this->Video.Duration);
}

RecordingState TwitchVideoInfo::GetVideoRecordingState() const {
    return this->Video.State;
}

DateTime TwitchVideoInfo::GetVideoTimestamp() const {
    return this->Video.PublishedAt.value_or(DateTime::FromBinary(0));
}

std::string_view TwitchVideoInfo::GetVideoTitle(std::array<char, 256>& buffer) const {
    if (this->Video.Title.has_value()) {
        return *this->Video.Title;
    }
    buffer[0] = '\0';
    return std::string_view(buffer.data(), 0);
}

VideoFileType TwitchVideoInfo::GetVideoType() const {
    return VideoFileType::M3U;
}

std::unique_ptr<IVideoInfo> TwitchVideoInfo::Clone() const {
    return std::make_unique<TwitchVideoInfo>(*this);
}
} // namespace VodArchiver
