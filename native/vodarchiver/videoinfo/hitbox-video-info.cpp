#include "hitbox-video-info.h"

#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "util/number.h"

namespace VodArchiver {
HitboxVideoInfo::~HitboxVideoInfo() = default;

StreamService HitboxVideoInfo::GetService() const {
    return StreamService::Hitbox;
}

std::string_view HitboxVideoInfo::GetUsername(std::array<char, 256>& buffer) const {
    return this->VideoInfo.MediaUserName;
}

std::string_view HitboxVideoInfo::GetVideoGame(std::array<char, 256>& buffer) const {
    return this->VideoInfo.MediaGame;
}

std::string_view HitboxVideoInfo::GetVideoId(std::array<char, 256>& buffer) const {
    auto result = std::format_to_n(buffer.data(), buffer.size() - 1, "{}", this->VideoInfo.MediaId);
    *result.out = '\0';
    return std::string_view(buffer.data(), result.out);
}

TimeSpan HitboxVideoInfo::GetVideoLength() const {
    return TimeSpan::FromDoubleSeconds(this->VideoInfo.MediaDuration);
}

RecordingState HitboxVideoInfo::GetVideoRecordingState() const {
    return RecordingState::Recorded;
}

DateTime HitboxVideoInfo::GetVideoTimestamp() const {
    return this->VideoInfo.MediaDateAdded;
}

std::string_view HitboxVideoInfo::GetVideoTitle(std::array<char, 256>& buffer) const {
    return this->VideoInfo.MediaTitle;
}

VideoFileType HitboxVideoInfo::GetVideoType() const {
    if (!VideoInfo.MediaProfiles.empty() && VideoInfo.MediaProfiles.front().Url.ends_with("m3u8")) {
        return VideoFileType::M3U;
    }
    return VideoFileType::Unknown;
}

std::unique_ptr<IVideoInfo> HitboxVideoInfo::Clone() const {
    return std::make_unique<HitboxVideoInfo>(*this);
}
} // namespace VodArchiver
