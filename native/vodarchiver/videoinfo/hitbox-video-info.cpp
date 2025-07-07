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

void HitboxVideoInfo::SetService(StreamService service) {
    throw std::runtime_error("cannot set service for HitboxVideoInfo");
}

std::string HitboxVideoInfo::GetUsername() const {
    return this->VideoInfo.MediaUserName;
}

void HitboxVideoInfo::SetUsername(std::string username) {
    this->VideoInfo.MediaUserName = std::move(username);
}

std::string HitboxVideoInfo::GetVideoGame() const {
    return this->VideoInfo.MediaGame;
}

void HitboxVideoInfo::SetVideoGame(std::string videoGame) {
    this->VideoInfo.MediaGame = std::move(videoGame);
}

std::string HitboxVideoInfo::GetVideoId() const {
    return std::format("{}", this->VideoInfo.MediaId);
}

void HitboxVideoInfo::SetVideoId(std::string videoId) {
    auto v = HyoutaUtils::NumberUtils::ParseInt32(videoId);
    if (!v) {
        throw std::runtime_error(std::format("failed to parse video ID '{}'", videoId));
    }
    this->VideoInfo.MediaId = *v;
}

TimeSpan HitboxVideoInfo::GetVideoLength() const {
    return TimeSpan::FromSeconds(this->VideoInfo.MediaDuration);
}

void HitboxVideoInfo::SetVideoLength(TimeSpan length) {
    this->VideoInfo.MediaDuration = length.GetTotalSeconds();
}

RecordingState HitboxVideoInfo::GetVideoRecordingState() const {
    return RecordingState::Recorded;
}

void HitboxVideoInfo::SetVideoRecordingState(RecordingState videoRecordingState) {
    throw std::runtime_error("cannot set recording state for HitboxVideoInfo");
}

DateTime HitboxVideoInfo::GetVideoTimestamp() const {
    return this->VideoInfo.MediaDateAdded;
}

void HitboxVideoInfo::SetVideoTimestamp(DateTime timestamp) {
    this->VideoInfo.MediaDateAdded = timestamp;
}

std::string HitboxVideoInfo::GetVideoTitle() const {
    return this->VideoInfo.MediaTitle;
}

void HitboxVideoInfo::SetVideoTitle(std::string videoTitle) {
    this->VideoInfo.MediaTitle = std::move(videoTitle);
}

VideoFileType HitboxVideoInfo::GetVideoType() const {
    if (!VideoInfo.MediaProfiles.empty() && VideoInfo.MediaProfiles.front().Url.ends_with("m3u8")) {
        return VideoFileType::M3U;
    }
    return VideoFileType::Unknown;
}

void HitboxVideoInfo::SetVideoType(VideoFileType videoType) {
    throw std::runtime_error("cannot set video type for HitboxVideoInfo");
}

std::unique_ptr<IVideoInfo> HitboxVideoInfo::Clone() const {
    return std::make_unique<HitboxVideoInfo>(*this);
}
} // namespace VodArchiver
