#pragma once

#include <string>

#include "i-user-info.h"

namespace VodArchiver {
struct YoutubeUrlUserInfo final : public IUserInfo {
    YoutubeUrlUserInfo();
    YoutubeUrlUserInfo(std::string url);
    ~YoutubeUrlUserInfo() override;

    ServiceVideoCategoryType GetType() override;
    std::string GetUserIdentifier() override;

    std::unique_ptr<IUserInfo> Clone() const override;

    FetchReturnValue Fetch(JobConfig& jobConfig, size_t offset, bool flat) override;

    std::string Url;
};
} // namespace VodArchiver
