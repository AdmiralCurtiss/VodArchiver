#include "twitch-user-info.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "vodarchiver/twitch_util.h"
#include "vodarchiver/videoinfo/i-video-info.h"
#include "vodarchiver/videoinfo/twitch-video-info.h"

namespace VodArchiver {
TwitchUserInfo::TwitchUserInfo() = default;

TwitchUserInfo::TwitchUserInfo(std::string username, bool highlights)
  : Username(std::move(username)), Highlights(highlights) {}

TwitchUserInfo::~TwitchUserInfo() = default;

ServiceVideoCategoryType TwitchUserInfo::GetType() {
    return Highlights ? ServiceVideoCategoryType::TwitchHighlights
                      : ServiceVideoCategoryType::TwitchRecordings;
}

std::string TwitchUserInfo::GetUserIdentifier() {
    return Username;
}

FetchReturnValue TwitchUserInfo::Fetch(size_t offset, bool flat) {
    std::vector<std::unique_ptr<IVideoInfo>> videosToAdd;
    bool hasMore = true;
    int64_t maxVideos = -1;
    int64_t currentVideos = -1;

    std::string twitchClientId = "";
    std::string twitchClientSecret = "";

    if (!UserID.has_value()) {
        auto uid = Twitch::GetUserIdFromUsername(Username, twitchClientId, twitchClientSecret);
        if (!uid) {
            return FetchReturnValue{.Success = false, .HasMore = false};
        }
        UserID = *uid;
    }

    auto broadcasts =
        Twitch::GetVideos(*UserID, Highlights, offset, 25, twitchClientId, twitchClientSecret);
    if (!broadcasts) {
        return FetchReturnValue{.Success = false, .HasMore = false};
    }
    hasMore = (offset + broadcasts->Videos.size()) < broadcasts->TotalVideoCount;
    maxVideos = broadcasts->TotalVideoCount;

    currentVideos = broadcasts->Videos.size();
    for (auto& v : broadcasts->Videos) {
        auto tv = std::make_unique<TwitchVideoInfo>();
        auto tc = std::make_unique<TwitchVideoInfo>();
        tv->_Service = StreamService::Twitch;
        tc->_Service = StreamService::TwitchChatReplay;
        tv->_Video = v;
        tc->_Video = std::move(v);
        videosToAdd.push_back(std::move(tv));
        videosToAdd.push_back(std::move(tc));
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
