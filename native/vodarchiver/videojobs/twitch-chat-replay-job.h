#pragma once

#include "i-video-job.h"

namespace VodArchiver {
struct TwitchChatReplayJob : public IVideoJob {
    bool IsWaitingForUserInput() const override;
    ResultType Run(TaskCancellation& cancellationToken) override;
    std::string GenerateOutputFilename() override;
};
} // namespace VodArchiver
