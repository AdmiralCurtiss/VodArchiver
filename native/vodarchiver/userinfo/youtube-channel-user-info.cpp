#include "youtube-channel-user-info.h"

#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "vodarchiver/youtube_util.h"

namespace VodArchiver {
YoutubeChannelUserInfo::YoutubeChannelUserInfo() = default;

YoutubeChannelUserInfo::YoutubeChannelUserInfo(std::string channel) : Channel(std::move(channel)) {}

YoutubeChannelUserInfo::~YoutubeChannelUserInfo() = default;

ServiceVideoCategoryType YoutubeChannelUserInfo::GetType() {
    return ServiceVideoCategoryType::YoutubeChannel;
}

std::string YoutubeChannelUserInfo::GetUserIdentifier() {
    return Channel;
}

FetchReturnValue YoutubeChannelUserInfo::Fetch(size_t offset, bool flat) {
    std::vector<std::unique_ptr<IVideoInfo>> videosToAdd;
    bool hasMore = true;
    int64_t maxVideos = -1;
    int64_t currentVideos = -1;

    auto videos = Youtube::RetrieveVideosFromChannel(Channel, flat, Comment);
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

std::string YoutubeChannelUserInfo::ToString() {
    if (!Comment.empty()) {
        return std::format(
            "{}: [{}] {}",
            ServiceVideoCategoryTypeToString(ServiceVideoCategoryType::YoutubeChannel),
            Comment,
            Channel);
    }
    return IUserInfo::ToString();
}
} // namespace VodArchiver
