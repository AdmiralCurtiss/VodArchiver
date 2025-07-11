#pragma once

#include "i-video-job.h"

namespace VodArchiver {
struct HitboxVideoJob : public IVideoJob {
    bool IsWaitingForUserInput() const override;
    ResultType Run(const std::string& targetFolderPath,
                   const std::string& tempFolderPath,
                   TaskCancellation& cancellationToken) override;
    std::string GenerateOutputFilename() override;

private:
    std::string GetTempFilenameWithoutExtension() const;
    std::string GetFinalFilenameWithoutExtension() const;
};
} // namespace VodArchiver
