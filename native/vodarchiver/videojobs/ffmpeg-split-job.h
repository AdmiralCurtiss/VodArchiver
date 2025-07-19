#pragma once

#include <memory>
#include <string>

#include "i-video-job.h"

namespace VodArchiver {
struct FFMpegSplitJob : public IVideoJob {
    bool IsWaitingForUserInput() const override;
    ResultType Run(JobConfig& jobConfig, TaskCancellation& cancellationToken) override;
    std::string GenerateOutputFilename() override;

    std::string SplitTimes;

    FFMpegSplitJob();
    FFMpegSplitJob(std::string path,
                   std::string splitTimes,
                   std::shared_ptr<IStatusUpdate> statusUpdater = nullptr);

private:
    std::string GenerateOutputName(std::string_view inputName);
};
} // namespace VodArchiver
