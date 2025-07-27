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
    std::string GetUsername() const override;
    std::string GetVideoId() const override;
    std::string GetVideoTitle() const override;
    std::string GetVideoGame() const override;
    DateTime GetVideoTimestamp() const override;
    TimeSpan GetVideoLength() const override;
    RecordingState GetVideoRecordingState() const override;
    VideoFileType GetVideoType() const override;

    std::unique_ptr<IVideoInfo> Clone() const override;

    HitboxVideo VideoInfo;
};
} // namespace VodArchiver
