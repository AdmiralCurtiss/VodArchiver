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

std::string HitboxVideoInfo::GetUsername() const {
    return this->VideoInfo.MediaUserName;
}

std::string HitboxVideoInfo::GetVideoGame() const {
    return this->VideoInfo.MediaGame;
}

std::string HitboxVideoInfo::GetVideoId() const {
    return std::format("{}", this->VideoInfo.MediaId);
}

TimeSpan HitboxVideoInfo::GetVideoLength() const {
    return TimeSpan::FromSeconds(this->VideoInfo.MediaDuration);
}

RecordingState HitboxVideoInfo::GetVideoRecordingState() const {
    return RecordingState::Recorded;
}

DateTime HitboxVideoInfo::GetVideoTimestamp() const {
    return this->VideoInfo.MediaDateAdded;
}

std::string HitboxVideoInfo::GetVideoTitle() const {
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
