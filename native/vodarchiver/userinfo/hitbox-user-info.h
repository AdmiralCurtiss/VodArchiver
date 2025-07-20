#pragma once

#include <string>

#include "i-user-info.h"

namespace VodArchiver {
struct HitboxUserInfo final : public IUserInfo {
    HitboxUserInfo();
    HitboxUserInfo(std::string username);
    ~HitboxUserInfo() override;

    ServiceVideoCategoryType GetType() override;
    std::string GetUserIdentifier() override;

    FetchReturnValue Fetch(JobConfig& jobConfig, size_t offset, bool flat) override;

    std::unique_ptr<IUserInfo> Clone() const override;

    std::string Username;
};
} // namespace VodArchiver
