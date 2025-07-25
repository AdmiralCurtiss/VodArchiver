#pragma once

#include <memory>

#include "i-video-job.h"

namespace VodArchiver {
struct TwitchChatReplayJob : public IVideoJob {
    bool IsWaitingForUserInput() const override;
    ResultType Run(JobConfig& jobConfig, TaskCancellation& cancellationToken) override;
    std::string GenerateOutputFilename() override;

    std::string GetTempFilenameWithoutExtension() const;
    std::string GetTargetFilenameWithoutExtension() const;

    std::shared_ptr<IUserInputRequest> GetUserInputRequest() const override;
    std::shared_ptr<IUserInputRequest> _UserInputRequest = nullptr;
    bool AssumeFinished = false;

    bool ShouldStallWrite(JobConfig& jobConfig,
                          std::string_view path,
                          uint64_t filesize) const override;
};
} // namespace VodArchiver
