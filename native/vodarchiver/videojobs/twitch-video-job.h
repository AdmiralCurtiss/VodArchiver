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
    ResultType Run(const std::string& targetFolderPath,
                   const std::string& tempFolderPath,
                   TaskCancellation& cancellationToken) override;

    std::string GenerateOutputFilename() override;

    std::string GetTempFilenameWithoutExtension() const;
    std::string GetFinalFilenameWithoutExtension() const;

    std::shared_ptr<IUserInputRequest> GetUserInputRequest() const override;
    std::shared_ptr<IUserInputRequest> _UserInputRequest = nullptr;
    bool _IsWaitingForUserInput = false;
    bool AssumeFinished = false;

    bool IgnoreTimeDifferenceCombined = false;
    bool IgnoreTimeDifferenceRemuxed = false;

    std::string VideoQuality = "chunked";

private:
    struct DownloadInfo {
        std::string Url;
        std::string FilesystemId;
        std::optional<uint64_t> Offset;
        std::optional<uint64_t> Length;
    };
    ResultType GetFileUrlsOfVod(std::vector<DownloadInfo>& downloadInfos,
                                const std::string& targetFolderPath,
                                const std::string& tempFolderPath,
                                TaskCancellation& cancellationToken);
    void GetFilenamesFromM3U8(std::vector<DownloadInfo>& downloadInfos,
                              const std::string& baseurl,
                              const std::string& m3u8);
    ResultType Download(std::vector<std::string>& files,
                        TaskCancellation& cancellationToken,
                        const std::string& targetFolder,
                        const std::vector<DownloadInfo>& downloadInfos,
                        int delayPerDownload = 0);
    ResultType Combine(TaskCancellation& cancellationToken,
                       const std::string& combinedFilename,
                       const std::vector<std::string>& files);
    bool Remux(const std::string& targetName,
               const std::string& sourceName,
               const std::string& tempName);
};
} // namespace VodArchiver
