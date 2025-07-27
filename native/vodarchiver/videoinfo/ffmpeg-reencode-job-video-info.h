#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "../ffmpeg_util.h"
#include "i-video-info.h"

namespace VodArchiver {
struct FFMpegReencodeJobVideoInfo : public IVideoInfo {
    FFMpegReencodeJobVideoInfo();
    FFMpegReencodeJobVideoInfo(std::string_view filename,
                               const FFProbeResult& probe,
                               std::vector<std::string> ffmpegOptions,
                               std::string postfixOld,
                               std::string postfixNew,
                               std::string outputformat);
    ~FFMpegReencodeJobVideoInfo() override;
    StreamService GetService() const override;
    std::string GetUsername() const override;
    std::string GetVideoId() const override;
    std::string GetVideoTitle() const override;
    std::string GetVideoGame() const override;
    DateTime GetVideoTimestamp() const override;
    TimeSpan GetVideoLength() const override;
    RecordingState GetVideoRecordingState() const override;
    VideoFileType GetVideoType() const override;

    std::unique_ptr<IVideoInfo> Clone() const override;

    std::vector<std::string> FFMpegOptions;
    std::string PostfixOld;
    std::string PostfixNew;
    std::string OutputFormat;
    uint64_t Filesize = 0;
    uint64_t Bitrate = 0;
    float Framerate = 0.0f;

    std::string VideoId;
    std::string VideoTitle;
    DateTime VideoTimestamp;
    TimeSpan VideoLength;
};
} // namespace VodArchiver
