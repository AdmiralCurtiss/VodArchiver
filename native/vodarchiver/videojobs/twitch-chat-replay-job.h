#pragma once

#include <memory>

#include "i-video-job.h"

namespace VodArchiver {
struct TwitchChatReplayJob : public IVideoJob {
    bool IsWaitingForUserInput() const override;
    ResultType Run(JobConfig& jobConfig, TaskCancellation& cancellationToken) override;
    std::string GenerateOutputFilename() override;
    std::unique_ptr<IVideoJob> Clone() const override;

    IUserInputRequest* GetUserInputRequest() const override;
    std::unique_ptr<IUserInputRequest> UserInputRequest = nullptr;
    bool AssumeFinished = false;
};
} // namespace VodArchiver
