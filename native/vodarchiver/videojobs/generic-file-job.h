#pragma once

#include "i-video-job.h"

namespace VodArchiver {
struct GenericFileJob : public IVideoJob {
    bool IsWaitingForUserInput() const override;
    ResultType Run(const std::string& targetFolderPath,
                   const std::string& tempFolderPath,
                   TaskCancellation& cancellationToken) override;
    std::string GenerateOutputFilename() override;

private:
    std::string GetTargetFilename() const;
};
} // namespace VodArchiver
