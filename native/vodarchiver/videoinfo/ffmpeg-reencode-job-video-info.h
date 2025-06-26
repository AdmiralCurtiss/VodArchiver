#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "i-video-info.h"

namespace VodArchiver {
struct FFMpegReencodeJobVideoInfo : public IVideoInfo {
    ~FFMpegReencodeJobVideoInfo() override;
    StreamService GetService() const override;
    void SetService(StreamService service) override;
    std::string GetUsername() const override;
    void SetUsername(std::string username) override;
    std::string GetVideoId() const override;
    void SetVideoId(std::string videoId) override;
    std::string GetVideoTitle() const override;
    void SetVideoTitle(std::string videoTitle) override;
    std::string GetVideoGame() const override;
    void SetVideoGame(std::string videoGame) override;
    DateTime GetVideoTimestamp() const override;
    void SetVideoTimestamp(DateTime timestamp) override;
    TimeSpan GetVideoLength() const override;
    void SetVideoLength(TimeSpan length) override;
    RecordingState GetVideoRecordingState() const override;
    void SetVideoRecordingState(RecordingState videoRecordingState) override;
    VideoFileType GetVideoType() const override;
    void SetVideoType(VideoFileType videoType) override;

    std::vector<std::string> FFMpegOptions;
    std::string PostfixOld;
    std::string PostfixNew;
    std::string OutputFormat;
    uint64_t Filesize;
    uint64_t Bitrate;
    float Framerate;

    std::string VideoId;
    std::string VideoTitle;
    DateTime VideoTimestamp;
    TimeSpan VideoLength;
};
} // namespace VodArchiver
