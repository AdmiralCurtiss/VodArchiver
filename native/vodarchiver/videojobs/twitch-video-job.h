#pragma once

#include <string>

#include "i-video-job.h"

namespace VodArchiver {
struct TwitchVideoJob : public IVideoJob {
    bool IsWaitingForUserInput() const override;
    ResultType Run(TaskCancellation& cancellationToken) override;
    std::string GenerateOutputFilename() override;

    std::string VideoQuality = "chunked";
};
} // namespace VodArchiver
