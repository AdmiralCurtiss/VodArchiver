#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "i-video-job.h"

namespace VodArchiver {
struct TwitchVideoJob : public IVideoJob {
    bool IsWaitingForUserInput() const override;
    ResultType Run(JobConfig& jobConfig, TaskCancellation& cancellationToken) override;
    std::string GenerateOutputFilename() override;

    std::shared_ptr<IUserInputRequest> GetUserInputRequest() const override;
    std::shared_ptr<IUserInputRequest> UserInputRequest = nullptr;
    bool WaitingForUserInput = false;
    bool AssumeFinished = false;

    bool IgnoreTimeDifferenceCombined = false;
    bool IgnoreTimeDifferenceRemuxed = false;

    std::string VideoQuality = "chunked";
};
} // namespace VodArchiver
