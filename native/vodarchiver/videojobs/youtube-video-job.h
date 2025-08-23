#pragma once

#include "i-video-job.h"

namespace VodArchiver {
struct YoutubeVideoJob : public IVideoJob {
    bool IsWaitingForUserInput() const override;
    ResultType Run(JobConfig& jobConfig, TaskCancellation& cancellationToken) override;
    std::string GenerateOutputFilename() override;
    std::unique_ptr<IVideoJob> Clone() const override;
};
} // namespace VodArchiver
