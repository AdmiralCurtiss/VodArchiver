#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "vodarchiver/time_types.h"
#include "vodarchiver/videoinfo/i-video-info.h"

namespace VodArchiver {
enum class ServiceVideoCategoryType : uint32_t {
    TwitchRecordings = 0,
    TwitchHighlights = 1,
    HitboxRecordings = 2,
    YoutubeUser = 3,
    YoutubeChannel = 4,
    YoutubePlaylist = 5,
    RssFeed = 6,
    FFMpegJob = 7,
    ArchiveOrg = 8,
    YoutubeUrl = 9,
    COUNT = 10,
};
std::string_view ServiceVideoCategoryTypeToString(ServiceVideoCategoryType type);
std::optional<ServiceVideoCategoryType> ServiceVideoCategoryTypeFromString(std::string_view sv);

struct FetchReturnValue {
    bool Success = false;
    bool HasMore = false;
    int64_t TotalVideos = 0;
    int64_t VideoCountThisFetch = 0;
    std::vector<std::unique_ptr<IVideoInfo>> Videos;
};

struct IUserInfo {
    virtual ~IUserInfo();

    virtual ServiceVideoCategoryType GetType() = 0;
    virtual std::string GetUserIdentifier() = 0;
    virtual std::string ToString();

    virtual FetchReturnValue Fetch(size_t offset, bool flat) = 0;

    bool Persistable = false;
    bool AutoDownload = false;
    DateTime LastRefreshedOn;
};
} // namespace VodArchiver
