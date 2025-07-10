#pragma once

#include <memory>

#include "i-video-job.h"

namespace VodArchiver {
struct FFMpegReencodeJob : public IVideoJob {
    bool IsWaitingForUserInput() const override;
    ResultType Run(const std::string& targetFolderPath,
                   const std::string& tempFolderPath,
                   TaskCancellation& cancellationToken) override;
    std::string GenerateOutputFilename() override;

    FFMpegReencodeJob();

private:
    bool Reencode(const std::string& targetName,
                  const std::string& sourceName,
                  const std::string& tempName,
                  const std::vector<std::string>& options);
};
} // namespace VodArchiver
