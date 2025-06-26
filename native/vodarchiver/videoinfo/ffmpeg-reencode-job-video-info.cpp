#include "ffmpeg-reencode-job-video-info.h"

#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace VodArchiver {
FFMpegReencodeJobVideoInfo::~FFMpegReencodeJobVideoInfo() = default;

StreamService FFMpegReencodeJobVideoInfo::GetService() const {
    return StreamService::FFMpegJob;
}

void FFMpegReencodeJobVideoInfo::SetService(StreamService service) {
    throw std::runtime_error("cannot set service for FFMpegReencodeJobVideoInfo");
}

RecordingState FFMpegReencodeJobVideoInfo::GetVideoRecordingState() const {
    return RecordingState::Recorded;
}

void FFMpegReencodeJobVideoInfo::SetVideoRecordingState(RecordingState videoRecordingState) {
    throw std::runtime_error("cannot set video recording state for FFMpegReencodeJobVideoInfo");
}

VideoFileType FFMpegReencodeJobVideoInfo::GetVideoType() const {
    return VideoFileType::Unknown;
}

void FFMpegReencodeJobVideoInfo::SetVideoType(VideoFileType videoType) {
    throw std::runtime_error("cannot set video type for FFMpegReencodeJobVideoInfo");
}

std::string FFMpegReencodeJobVideoInfo::GetUsername() const {
    return this->PostfixNew;
}

void FFMpegReencodeJobVideoInfo::SetUsername(std::string username) {
    throw std::runtime_error("cannot set username for FFMpegReencodeJobVideoInfo");
}

std::string FFMpegReencodeJobVideoInfo::GetVideoGame() const {
    return std::format("{} MB; {} kbps, {} fps", Filesize / 1000000, Bitrate / 1000, Framerate);
}

void FFMpegReencodeJobVideoInfo::SetVideoGame(std::string videoGame) {
    throw std::runtime_error("cannot set game for FFMpegReencodeJobVideoInfo");
}

std::string FFMpegReencodeJobVideoInfo::GetVideoId() const {
    return this->VideoId;
}

void FFMpegReencodeJobVideoInfo::SetVideoId(std::string videoId) {
    this->VideoId = std::move(videoId);
}

std::string FFMpegReencodeJobVideoInfo::GetVideoTitle() const {
    return this->VideoTitle;
}

void FFMpegReencodeJobVideoInfo::SetVideoTitle(std::string videoTitle) {
    this->VideoTitle = std::move(videoTitle);
}

DateTime FFMpegReencodeJobVideoInfo::GetVideoTimestamp() const {
    return this->VideoTimestamp;
}

void FFMpegReencodeJobVideoInfo::SetVideoTimestamp(DateTime timestamp) {
    this->VideoTimestamp = timestamp;
}

TimeSpan FFMpegReencodeJobVideoInfo::GetVideoLength() const {
    return this->VideoLength;
}

void FFMpegReencodeJobVideoInfo::SetVideoLength(TimeSpan length) {
    this->VideoLength = length;
}
} // namespace VodArchiver
