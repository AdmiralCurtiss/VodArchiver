#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "vodarchiver/videoinfo/i-video-info.h"
#include "vodarchiver/videoinfo/twitch-video-info.h"

namespace VodArchiver::Twitch {
std::optional<int64_t> GetUserIdFromUsername(const std::string& username,
                                             const std::string& clientId,
                                             const std::string& clientSecret);
struct TwitchVodFetchResult {
    std::vector<TwitchVideo> Videos;
    int64_t TotalVideoCount;
};
std::optional<TwitchVodFetchResult> GetVideos(int64_t channelId,
                                              bool highlights,
                                              int offset,
                                              int limit,
                                              const std::string& clientId,
                                              const std::string& clientSecret);
} // namespace VodArchiver::Twitch
