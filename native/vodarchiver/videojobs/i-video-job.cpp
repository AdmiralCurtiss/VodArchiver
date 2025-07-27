#include "i-video-job.h"

#include <algorithm>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string_view>

#include "util/text.h"

#include "vodarchiver/job_config.h"
#include "vodarchiver/system_util.h"

namespace VodArchiver {
std::string_view VideoJobStatusToString(VideoJobStatus status) {
    switch (status) {
        case VideoJobStatus::NotStarted: return "NotStarted";
        case VideoJobStatus::Running: return "Running";
        case VideoJobStatus::Finished: return "Finished";
        case VideoJobStatus::Dead: return "Dead";
        default: return "Unknown";
    }
}

std::optional<VideoJobStatus> VideoJobStatusFromString(std::string_view sv) {
    if (sv == "NotStarted") {
        return VideoJobStatus::NotStarted;
    } else if (sv == "Running") {
        return VideoJobStatus::Running;
    } else if (sv == "Finished") {
        return VideoJobStatus::Finished;
    } else if (sv == "Dead") {
        return VideoJobStatus::Dead;
    }
    return std::nullopt;
}

std::string_view ResultTypeToString(ResultType type) {
    switch (type) {
        case ResultType::Failure: return "Failure";
        case ResultType::Success: return "Success";
        case ResultType::Cancelled: return "Cancelled";
        case ResultType::Dead: return "Dead";
        case ResultType::UserInputRequired: return "UserInputRequired";
        case ResultType::TemporarilyUnavailable: return "TemporarilyUnavailable";
        default: return "Unknown";
    }
}

std::optional<ResultType> ResultTypeFromString(std::string_view sv) {
    if (sv == "Failure") {
        return ResultType::Failure;
    } else if (sv == "Success") {
        return ResultType::Success;
    } else if (sv == "Cancelled") {
        return ResultType::Cancelled;
    } else if (sv == "Dead") {
        return ResultType::Dead;
    } else if (sv == "UserInputRequired") {
        return ResultType::UserInputRequired;
    } else if (sv == "TemporarilyUnavailable") {
        return ResultType::TemporarilyUnavailable;
    }
    return std::nullopt;
}

IUserInputRequest::~IUserInputRequest() = default;

IVideoJob::~IVideoJob() = default;

void IVideoJob::SetStatus(std::string value) {
    TextStatus = value;
}

std::shared_ptr<IUserInputRequest> IVideoJob::GetUserInputRequest() const {
    return nullptr;
}

std::string IVideoJob::GetHumanReadableJobName() const {
    IVideoInfo* videoInfo = this->VideoInfo.get();
    if (videoInfo) {
        std::string username = videoInfo->GetUsername();
        if (!HyoutaUtils::TextUtils::Trim(username).empty()) {
            return username + "/" + videoInfo->GetVideoId() + " ("
                   + std::string(StreamServiceToString(videoInfo->GetService())) + ")";
        } else {
            return videoInfo->GetVideoId() + " ("
                   + std::string(StreamServiceToString(videoInfo->GetService())) + ")";
        }
    } else {
        return "Unknown Video";
    }
}

bool ShouldStallWriteRegularFile(JobConfig& jobConfig, std::string_view path, uint64_t filesize) {
    auto freeSpace = GetFreeDiskSpaceAtPath(path);
    if (!freeSpace.has_value()) {
        return false;
    }

    std::lock_guard lock(jobConfig.Mutex);
    return *freeSpace <= (jobConfig.MinimumFreeSpaceBytes + filesize);
}
bool ShouldStallWriteSmallFile(JobConfig& jobConfig, std::string_view path, uint64_t filesize) {
    auto freeSpace = GetFreeDiskSpaceAtPath(path);
    if (!freeSpace.has_value()) {
        return false;
    }

    std::lock_guard lock(jobConfig.Mutex);
    return *freeSpace
           <= (std::min(jobConfig.AbsoluteMinimumFreeSpaceBytes, jobConfig.MinimumFreeSpaceBytes)
               + filesize);
}

void StallWrite(
    JobConfig& jobConfig,
    std::string_view path,
    uint64_t filesize,
    TaskCancellation& cancellationToken,
    const std::function<bool(JobConfig& jobConfig, std::string_view path, uint64_t filesize)>&
        shouldStallWriteCallback,
    const std::function<void(std::string status)>& setStatusCallback) {
    if (shouldStallWriteCallback(jobConfig, path, filesize)) {
        setStatusCallback("Not enough free space, stalling...");
        do {
            if (cancellationToken.IsCancellationRequested()) {
                return;
            }
            cancellationToken.DelayFor(std::chrono::seconds(10));
        } while (shouldStallWriteCallback(jobConfig, path, filesize));
    }
}

} // namespace VodArchiver
