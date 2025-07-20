#pragma once

#include <string>

#include "i-user-info.h"

namespace VodArchiver {
struct YoutubeUserUserInfo final : public IUserInfo {
    YoutubeUserUserInfo();
    YoutubeUserUserInfo(std::string username);
    ~YoutubeUserUserInfo() override;

    ServiceVideoCategoryType GetType() override;
    std::string GetUserIdentifier() override;

    std::unique_ptr<IUserInfo> Clone() const override;

    FetchReturnValue Fetch(JobConfig& jobConfig, size_t offset, bool flat) override;

    std::string Username;
};
} // namespace VodArchiver
