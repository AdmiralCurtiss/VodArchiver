#include "youtube-playlist-user-info.h"

#include <format>
#include <string>
#include <utility>

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

FetchReturnValue YoutubePlaylistUserInfo::Fetch(size_t offset, bool flat) {
    throw "not implemented";
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
} // namespace VodArchiver
