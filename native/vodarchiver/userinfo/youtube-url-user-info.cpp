#include "youtube-url-user-info.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "vodarchiver/youtube_util.h"

namespace VodArchiver {
YoutubeUrlUserInfo::YoutubeUrlUserInfo() = default;

YoutubeUrlUserInfo::YoutubeUrlUserInfo(std::string url) : Url(std::move(url)) {}

YoutubeUrlUserInfo::~YoutubeUrlUserInfo() = default;

ServiceVideoCategoryType YoutubeUrlUserInfo::GetType() {
    return ServiceVideoCategoryType::YoutubeUrl;
}

std::string YoutubeUrlUserInfo::GetUserIdentifier() {
    return Url;
}

FetchReturnValue YoutubeUrlUserInfo::Fetch(JobConfig& jobConfig, size_t offset, bool flat) {
    std::vector<std::unique_ptr<IVideoInfo>> videosToAdd;
    bool hasMore = true;
    int64_t maxVideos = -1;
    int64_t currentVideos = -1;

    auto videos = Youtube::RetrieveVideosFromUrl(Url, flat);
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
