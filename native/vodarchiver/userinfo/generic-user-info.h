#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "i-user-info.h"

namespace VodArchiver {
struct GenericUserInfo final : public IUserInfo {
    ~GenericUserInfo() override;

    ServiceVideoCategoryType GetType() override;
    std::string GetUserIdentifier() override;

    FetchReturnValue Fetch(JobConfig& jobConfig, size_t offset, bool flat) override;

    std::unique_ptr<IUserInfo> Clone() const override;

    ServiceVideoCategoryType Service;
    std::optional<int64_t> UserID;
    std::string Username;
    std::string AdditionalOptions;
};
} // namespace VodArchiver
