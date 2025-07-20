#include "youtube-playlist-user-info.h"

#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "vodarchiver/youtube_util.h"

namespace VodArchiver {
YoutubePlaylistUserInfo::YoutubePlaylistUserInfo() = default;

YoutubePlaylistUserInfo::YoutubePlaylistUserInfo(std::string playlist)
  : Playlist(std::move(playlist)) {}

YoutubePlaylistUserInfo::~YoutubePlaylistUserInfo() = default;

ServiceVideoCategoryType YoutubePlaylistUserInfo::GetType() {
    return ServiceVideoCategoryType::YoutubePlaylist;
}

std::string YoutubePlaylistUserInfo::GetUserIdentifier() {
    return Playlist;
}

FetchReturnValue YoutubePlaylistUserInfo::Fetch(JobConfig& jobConfig, size_t offset, bool flat) {
    std::vector<std::unique_ptr<IVideoInfo>> videosToAdd;
    bool hasMore = true;
    int64_t maxVideos = -1;
    int64_t currentVideos = -1;

    auto videos = Youtube::RetrieveVideosFromPlaylist(Playlist, flat, Comment);
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

std::string YoutubePlaylistUserInfo::ToString() {
    if (!Comment.empty()) {
        return std::format(
            "{}: [{}] {}",
            ServiceVideoCategoryTypeToString(ServiceVideoCategoryType::YoutubePlaylist),
            Comment,
            Playlist);
    }
    return IUserInfo::ToString();
}

std::unique_ptr<IUserInfo> YoutubePlaylistUserInfo::Clone() const {
    auto u = std::make_unique<YoutubePlaylistUserInfo>();
    u->Persistable = this->Persistable;
    u->AutoDownload = this->AutoDownload;
    u->LastRefreshedOn = this->LastRefreshedOn;
    u->Playlist = this->Playlist;
    u->Comment = this->Comment;
    return u;
}
} // namespace VodArchiver
