#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "i-video-info.h"

#include "../time_types.h"

namespace VodArchiver {
struct HitboxMediaProfile {
    std::string Url;
    int Height;
    int Bitrate;
};

struct HitboxVideo {
    std::string MediaUserName;
    int32_t MediaId;
    std::string MediaFile;
    int32_t MediaUserId;
    std::vector<HitboxMediaProfile> MediaProfiles;
    DateTime MediaDateAdded;
    std::string MediaTitle;
    std::string MediaDescription;
    std::string MediaGame;
    double MediaDuration;
    int32_t MediaTypeId;
};

struct HitboxVideoInfo : public IVideoInfo {
    ~HitboxVideoInfo() override;
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

    HitboxVideo VideoInfo;
};
} // namespace VodArchiver
