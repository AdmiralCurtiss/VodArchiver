#pragma once

#include <string>

#include "i-user-info.h"

namespace VodArchiver {
struct RssFeedUserInfo final : public IUserInfo {
    RssFeedUserInfo();
    RssFeedUserInfo(std::string url);
    ~RssFeedUserInfo() override;

    ServiceVideoCategoryType GetType() override;
    std::string GetUserIdentifier() override;

    FetchReturnValue Fetch(size_t offset, bool flat) override;

    std::string Url;
};
} // namespace VodArchiver
