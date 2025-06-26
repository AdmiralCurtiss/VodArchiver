#pragma once

#include <string>

#include "i-video-job.h"

namespace VodArchiver {
struct FFMpegSplitJob : public IVideoJob {
    bool IsWaitingForUserInput() const override;
    ResultType Run(TaskCancellation& cancellationToken) override;
    std::string GenerateOutputFilename() override;

    std::string SplitTimes;
};
} // namespace VodArchiver
