#pragma once

#include <string>

#include "i-user-info.h"

namespace VodArchiver {
struct ArchiveOrgUserInfo final : public IUserInfo {
    ArchiveOrgUserInfo();
    ArchiveOrgUserInfo(std::string url);
    ~ArchiveOrgUserInfo() override;

    ServiceVideoCategoryType GetType() override;
    std::string GetUserIdentifier() override;

    FetchReturnValue Fetch(JobConfig& jobConfig, size_t offset, bool flat) override;

    std::unique_ptr<IUserInfo> Clone() const override;

    std::string Identifier;
};
} // namespace VodArchiver
