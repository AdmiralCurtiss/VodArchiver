#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "i-user-info.h"

namespace VodArchiver {
struct TwitchUserInfo final : public IUserInfo {
    TwitchUserInfo();
    TwitchUserInfo(std::string username, bool highlights);
    ~TwitchUserInfo() override;

    ServiceVideoCategoryType GetType() override;
    std::string GetUserIdentifier() override;

    FetchReturnValue Fetch(JobConfig& jobConfig, size_t offset, bool flat) override;

    std::unique_ptr<IUserInfo> Clone() const override;

    std::string Username;
    std::optional<int64_t> UserID;
    bool Highlights = false;
};
} // namespace VodArchiver
