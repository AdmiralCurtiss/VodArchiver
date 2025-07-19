#include "youtube-user-user-info.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "vodarchiver/youtube_util.h"

namespace VodArchiver {
YoutubeUserUserInfo::YoutubeUserUserInfo() = default;

YoutubeUserUserInfo::YoutubeUserUserInfo(std::string username) : Username(std::move(username)) {}

YoutubeUserUserInfo::~YoutubeUserUserInfo() = default;

ServiceVideoCategoryType YoutubeUserUserInfo::GetType() {
    return ServiceVideoCategoryType::YoutubeUser;
}

std::string YoutubeUserUserInfo::GetUserIdentifier() {
    return Username;
}

FetchReturnValue YoutubeUserUserInfo::Fetch(JobConfig& jobConfig, size_t offset, bool flat) {
    std::vector<std::unique_ptr<IVideoInfo>> videosToAdd;
    bool hasMore = true;
    int64_t maxVideos = -1;
    int64_t currentVideos = -1;

    auto videos = Youtube::RetrieveVideosFromUser(Username, flat);
    if (!videos.has_value()) {
        return FetchReturnValue{.Success = false, .HasMore = false};
    }
    hasMore = false;
    currentVideos = videos->size();
    for (auto& m : *videos) {
        videosToAdd.push_back(std::move(m));
    }

    if (videosToAdd.size() <= 0) {
        return FetchReturnValue{.Success = true,
                                .HasMore = false,
                                .TotalVideos = maxVideos,
                                .VideoCountThisFetch = 0,
                                .Videos = std::move(videosToAdd)};
    }

    return FetchReturnValue{.Success = true,
                            .HasMore = hasMore,
                            .TotalVideos = maxVideos,
                            .VideoCountThisFetch = currentVideos,
                            .Videos = std::move(videosToAdd)};
}
} // namespace VodArchiver
