#pragma once

#include <string>

#include "i-video-job.h"

namespace VodArchiver {
struct TwitchVideoJob : public IVideoJob {
    bool IsWaitingForUserInput() const override;
    ResultType Run(const std::string& targetFolderPath,
                   const std::string& tempFolderPath,
                   TaskCancellation& cancellationToken) override;
    std::string GenerateOutputFilename() override;

    std::string VideoQuality = "chunked";
};
} // namespace VodArchiver
