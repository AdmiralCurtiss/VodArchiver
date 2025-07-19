#pragma once

#include "i-video-job.h"

namespace VodArchiver {
struct GenericFileJob : public IVideoJob {
    bool IsWaitingForUserInput() const override;
    ResultType Run(JobConfig& jobConfig, TaskCancellation& cancellationToken) override;
    std::string GenerateOutputFilename() override;

private:
    std::string GetTargetFilename() const;
};
} // namespace VodArchiver
