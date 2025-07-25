#pragma once

#include "i-video-job.h"

namespace VodArchiver {
struct HitboxVideoJob : public IVideoJob {
    bool IsWaitingForUserInput() const override;
    ResultType Run(JobConfig& jobConfig, TaskCancellation& cancellationToken) override;
    std::string GenerateOutputFilename() override;

private:
    std::string GetTempFilenameWithoutExtension() const;
    std::string GetFinalFilenameWithoutExtension() const;
};
} // namespace VodArchiver
