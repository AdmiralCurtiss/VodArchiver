#pragma once

#include <string>

#include "i-user-info.h"

namespace VodArchiver {
struct FFMpegJobUserInfo final : public IUserInfo {
    FFMpegJobUserInfo();
    FFMpegJobUserInfo(std::string path, std::string preset);
    ~FFMpegJobUserInfo() override;

    ServiceVideoCategoryType GetType() override;
    std::string GetUserIdentifier() override;

    FetchReturnValue Fetch(JobConfig& jobConfig, size_t offset, bool flat) override;

    std::string Path;
    std::string Preset;
};
} // namespace VodArchiver
