#pragma once

#include <memory>

#include "i-video-job.h"

namespace VodArchiver {
struct FFMpegReencodeJob : public IVideoJob {
    bool IsWaitingForUserInput() const override;
    ResultType Run(JobConfig& jobConfig, TaskCancellation& cancellationToken) override;
    std::string GenerateOutputFilename() override;

    FFMpegReencodeJob();
};
} // namespace VodArchiver
