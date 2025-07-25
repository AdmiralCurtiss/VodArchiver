#pragma once

#include <string>

#include "i-user-info.h"

namespace VodArchiver {
struct YoutubePlaylistUserInfo final : public IUserInfo {
    YoutubePlaylistUserInfo();
    YoutubePlaylistUserInfo(std::string playlist);
    ~YoutubePlaylistUserInfo() override;

    ServiceVideoCategoryType GetType() override;
    std::string GetUserIdentifier() override;

    FetchReturnValue Fetch(JobConfig& jobConfig, size_t offset, bool flat) override;

    std::unique_ptr<IUserInfo> Clone() const override;

    std::string ToString() override;

    std::string Playlist;
    std::string Comment;
};
} // namespace VodArchiver
