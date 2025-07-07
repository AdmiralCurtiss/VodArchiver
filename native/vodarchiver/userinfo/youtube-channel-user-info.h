#pragma once

#include <string>

#include "i-user-info.h"

namespace VodArchiver {
struct YoutubeChannelUserInfo final : public IUserInfo {
    YoutubeChannelUserInfo();
    YoutubeChannelUserInfo(std::string channel);
    ~YoutubeChannelUserInfo() override;

    ServiceVideoCategoryType GetType() override;
    std::string GetUserIdentifier() override;

    FetchReturnValue Fetch(size_t offset, bool flat) override;

    std::string ToString() override;

    std::string Channel;
    std::string Comment;
};
} // namespace VodArchiver
