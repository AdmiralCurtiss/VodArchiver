#include "generic-file-job.h"

#include <atomic>
#include <cstddef>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>

#include "util/file.h"
#include "util/text.h"

#include "vodarchiver/curl_util.h"
#include "vodarchiver/filename_util.h"

namespace VodArchiver {
static std::atomic<size_t> s_Counter = 0;

bool GenericFileJob::IsWaitingForUserInput() const {
    return false;
}

static std::string StringToFilename(std::string_view basename, std::string_view extension) {
    return std::format("{}.{}", MakeStringFileSystemSafeBaseName(basename), extension);
}

static std::string GetTempFoldername() {
    size_t c = s_Counter.fetch_add(1);
    return std::format("rawurl_{}", c);
}

static std::string GetTargetFilename(IVideoInfo& videoInfo) {
    // best guess...
    std::array<char, 256> buffer1;
    std::array<char, 256> buffer2;
    std::array<char, 256> buffer3;
    std::string_view videoId = videoInfo.GetVideoId(buffer1);
    std::string_view rawUsername = videoInfo.GetUsername(buffer2);
    std::string_view rawTitle = videoInfo.GetVideoTitle(buffer3);
    DateTime rawTimestamp = videoInfo.GetVideoTimestamp();
    std::string extension;
    {
        auto slash = HyoutaUtils::TextUtils::Split(HyoutaUtils::TextUtils::Trim(videoId), "/");
        if (!slash.empty()) {
            auto que = HyoutaUtils::TextUtils::Split(slash.back(), "?");
            if (!que.empty()) {
                auto underscore = HyoutaUtils::TextUtils::Split(
                    MakeStringFileSystemSafeBaseName(que.front()), "_");
                if (!underscore.empty()) {
                    extension = std::string(HyoutaUtils::TextUtils::Trim(underscore.back()));
                }
            }
        }
    }
    if (extension.empty()) {
        extension = "bin";
    }

    std::string_view username = HyoutaUtils::TextUtils::Trim(rawUsername);
    std::string_view fallbackTitle = "file";
    std::string_view title = HyoutaUtils::TextUtils::Trim(rawTitle).empty()
                                 ? fallbackTitle
                                 : HyoutaUtils::TextUtils::Trim(rawTitle);
    std::string datetime;
    if (!(rawTimestamp.Data == 0 || rawTimestamp.Data == DateTime::FromUnixTime(0).Data)) {
        datetime = DateTimeToStringForFilesystem(rawTimestamp);
    }
    std::string prefix = HyoutaUtils::TextUtils::Trim(username).empty() ? datetime
                         : HyoutaUtils::TextUtils::Trim(datetime).empty()
                             ? std::string(username)
                             : std::format("{}_{}", username, datetime);
    std::string basename = HyoutaUtils::TextUtils::Trim(prefix).empty()
                               ? std::string(title)
                               : std::format("{}_{}", prefix, title);

    return StringToFilename(basename, extension);
}

static std::string PathCombine(std::string_view lhs, std::string_view rhs) {
    std::string result(lhs);
    HyoutaUtils::IO::AppendPathElement(result, rhs);
    return result;
}

static bool DeleteDirectoryRecursive(const std::filesystem::path& rootDir) {
    {
        std::error_code ec;
        std::filesystem::directory_iterator iterator(rootDir, ec);
        if (ec) {
            return false;
        }
        while (iterator != std::filesystem::directory_iterator()) {
            if (iterator->is_directory()) {
                if (!DeleteDirectoryRecursive(*iterator)) {
                    return false;
                }
            } else {
                if (!HyoutaUtils::IO::DeleteFile(*iterator)) {
                    return false;
                }
            }
            iterator.increment(ec);
            if (ec) {
                return false;
            }
        }
    }

    return HyoutaUtils::IO::DeleteDirectory(rootDir);
}

static bool DeleteDirectoryRecursive(std::string_view path) {
    return DeleteDirectoryRecursive(HyoutaUtils::IO::FilesystemPathFromUtf8(path));
}

static ResultType RunGenericFileJob(GenericFileJob& job,
                                    JobConfig& jobConfig,
                                    TaskCancellation& cancellationToken) {
    std::unique_ptr<IVideoInfo> videoInfo;
    {
        std::lock_guard lock(*jobConfig.JobsLock);
        job.JobStatus = VideoJobStatus::Running;
        job.TextStatus = "Downloading...";
        videoInfo = job.VideoInfo->Clone();
    }

    std::string tempFolderPath;
    std::string targetFolderPath;
    {
        std::lock_guard lock(jobConfig.Mutex);
        tempFolderPath = jobConfig.TempFolderPath;
        targetFolderPath = jobConfig.TargetFolderPath;
    }

    std::string tempFoldername = PathCombine(tempFolderPath, GetTempFoldername());
    std::string urlFilename = "url.txt";
    std::string urlFilepath = PathCombine(tempFoldername, urlFilename);
    std::string downloadFilename = "wip.bin";
    std::string downloadFilepath = PathCombine(tempFoldername, downloadFilename);
    std::string progressFilename = "progress.txt";
    std::string progressFilepath = PathCombine(tempFoldername, progressFilename);
    std::string movedFilename = "done.bin";
    std::string movedFilepath = PathCombine(tempFoldername, movedFilename);
    std::string targetFilename = GetTargetFilename(*videoInfo);
    std::string targetFilepath = PathCombine(targetFolderPath, targetFilename);

    if (!HyoutaUtils::IO::Exists(std::string_view(targetFilepath))) {
        if (!HyoutaUtils::IO::Exists(std::string_view(movedFilepath))) {
            if (HyoutaUtils::IO::DirectoryExists(std::string_view(tempFoldername))) {
                if (!DeleteDirectoryRecursive(std::string_view(tempFoldername))) {
                    return ResultType::Failure;
                }
            }
            if (!HyoutaUtils::IO::CreateDirectory(std::string_view(tempFoldername))) {
                return ResultType::Failure;
            }

            if (cancellationToken.IsCancellationRequested()) {
                return ResultType::Cancelled;
            }

            std::array<char, 256> buffer;
            std::string videoId(videoInfo->GetVideoId(buffer));
            if (!HyoutaUtils::IO::WriteFileAtomic(urlFilepath, videoId.data(), videoId.size())) {
                return ResultType::Failure;
            }

            auto response = VodArchiver::curl::GetFromUrlToMemory(videoId);
            if (!response || response->ResponseCode >= 400) {
                return ResultType::Failure;
            }
            if (!HyoutaUtils::IO::WriteFileAtomic(
                    downloadFilepath, response->Data.data(), response->Data.size())) {
                return ResultType::Failure;
            }

            if (cancellationToken.IsCancellationRequested()) {
                return ResultType::Cancelled;
            }

            if (!HyoutaUtils::IO::Move(downloadFilepath, movedFilepath)) {
                return ResultType::Failure;
            }
        }

        if (cancellationToken.IsCancellationRequested()) {
            return ResultType::Cancelled;
        }
        StallWrite(jobConfig,
                   targetFilepath,
                   HyoutaUtils::IO::GetFilesize(std::string_view(movedFilepath)).value_or(0),
                   cancellationToken,
                   ShouldStallWriteRegularFile,
                   [&](std::string status) {
                       std::lock_guard lock(*jobConfig.JobsLock);
                       job.TextStatus = std::move(status);
                   });
        if (cancellationToken.IsCancellationRequested()) {
            return ResultType::Cancelled;
        }
        if (!HyoutaUtils::IO::Move(movedFilepath, targetFilepath)) {
            return ResultType::Failure;
        }
        if (!DeleteDirectoryRecursive(std::string_view(tempFoldername))) {
            return ResultType::Failure;
        }
    }

    {
        std::lock_guard lock(*jobConfig.JobsLock);
        job.TextStatus = "Done!";
        job.JobStatus = VideoJobStatus::Finished;
    }
    return ResultType::Success;
}

ResultType GenericFileJob::Run(JobConfig& jobConfig, TaskCancellation& cancellationToken) {
    return RunGenericFileJob(*this, jobConfig, cancellationToken);
}

std::string GenericFileJob::GenerateOutputFilename() {
    return GetTargetFilename(*this->VideoInfo);
}
} // namespace VodArchiver
