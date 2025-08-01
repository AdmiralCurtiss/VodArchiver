#include "ffmpeg-reencode-job-video-info.h"

#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "util/file.h"

namespace VodArchiver {
FFMpegReencodeJobVideoInfo::FFMpegReencodeJobVideoInfo() {}

FFMpegReencodeJobVideoInfo::FFMpegReencodeJobVideoInfo(std::string_view filename,
                                                       const FFProbeResult& probe,
                                                       std::vector<std::string> ffmpegOptions,
                                                       std::string postfixOld,
                                                       std::string postfixNew,
                                                       std::string outputformat) {
    VideoTitle = HyoutaUtils::IO::GetFileNameWithoutExtension(filename);
    VideoId = HyoutaUtils::IO::GetAbsolutePath(filename);
    Filesize = probe.Filesize;
    Bitrate = probe.Bitrate;
    Framerate = 0.0f;
    for (auto& s : probe.Streams) {
        if (s.Framerate > 0.0f) {
            Framerate = s.Framerate;
            break;
        }
    }
    VideoTimestamp = probe.Timestamp;
    VideoLength = probe.Duration;
    FFMpegOptions = std::move(ffmpegOptions);
    PostfixOld = std::move(postfixOld);
    PostfixNew = std::move(postfixNew);
    OutputFormat = std::move(outputformat);
}

FFMpegReencodeJobVideoInfo::~FFMpegReencodeJobVideoInfo() = default;

StreamService FFMpegReencodeJobVideoInfo::GetService() const {
    return StreamService::FFMpegJob;
}

RecordingState FFMpegReencodeJobVideoInfo::GetVideoRecordingState() const {
    return RecordingState::Recorded;
}

VideoFileType FFMpegReencodeJobVideoInfo::GetVideoType() const {
    return VideoFileType::Unknown;
}

std::string_view FFMpegReencodeJobVideoInfo::GetUsername(std::array<char, 256>& buffer) const {
    return this->PostfixNew;
}

std::string_view FFMpegReencodeJobVideoInfo::GetVideoGame(std::array<char, 256>& buffer) const {
    auto result = std::format_to_n(buffer.data(),
                                   buffer.size() - 1,
                                   "{} MB; {} kbps, {} fps",
                                   Filesize / 1000000,
                                   Bitrate / 1000,
                                   Framerate);
    *result.out = '\0';
    return std::string_view(buffer.data(), result.out);
}

std::string_view FFMpegReencodeJobVideoInfo::GetVideoId(std::array<char, 256>& buffer) const {
    return this->VideoId;
}

std::string_view FFMpegReencodeJobVideoInfo::GetVideoTitle(std::array<char, 256>& buffer) const {
    return this->VideoTitle;
}

DateTime FFMpegReencodeJobVideoInfo::GetVideoTimestamp() const {
    return this->VideoTimestamp;
}

TimeSpan FFMpegReencodeJobVideoInfo::GetVideoLength() const {
    return this->VideoLength;
}

std::unique_ptr<IVideoInfo> FFMpegReencodeJobVideoInfo::Clone() const {
    return std::make_unique<FFMpegReencodeJobVideoInfo>(*this);
}
} // namespace VodArchiver
