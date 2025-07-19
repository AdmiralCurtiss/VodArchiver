#include "i-video-job.h"

#include "util/text.h"

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

const std::string& IVideoJob::GetStatus() const {
    return _Status;
}

void IVideoJob::SetStatus(std::string value) {
    _Status = value;
    if (StatusUpdater) {
        StatusUpdater->Update();
    }
}

std::shared_ptr<IUserInputRequest> IVideoJob::GetUserInputRequest() const {
    return nullptr;
}

std::string IVideoJob::GetHumanReadableJobName() const {
    auto VideoInfo = GetVideoInfo();
    if (VideoInfo) {
        std::string username = VideoInfo->GetUsername();
        if (!HyoutaUtils::TextUtils::Trim(username).empty()) {
            return username + "/" + VideoInfo->GetVideoId() + " ("
                   + std::string(StreamServiceToString(VideoInfo->GetService())) + ")";
        } else {
            return VideoInfo->GetVideoId() + " ("
                   + std::string(StreamServiceToString(VideoInfo->GetService())) + ")";
        }
    } else {
        return "Unknown Video";
    }
}

std::shared_ptr<IVideoInfo> IVideoJob::GetVideoInfo() const {
    return _VideoInfo;
}

void IVideoJob::SetVideoInfo(std::shared_ptr<IVideoInfo> videoInfo) {
    _VideoInfo = videoInfo;
}

bool IVideoJob::ShouldStallWrite(JobConfig& jobConfig,
                                 std::string_view path,
                                 uint64_t filesize) const {
    auto freeSpace = GetFreeDiskSpaceAtPath(path);
    if (!freeSpace.has_value()) {
        return false;
    }

    std::lock_guard lock(jobConfig.Mutex);
    return *freeSpace <= (jobConfig.MinimumFreeSpaceBytes + filesize);
}

void IVideoJob::StallWrite(JobConfig& jobConfig,
                           std::string_view path,
                           uint64_t filesize,
                           TaskCancellation& cancellationToken) {
    if (ShouldStallWrite(jobConfig, path, filesize)) {
        SetStatus("Not enough free space, stalling...");
        do {
            if (cancellationToken.IsCancellationRequested()) {
                return;
            }
            // TODO
            // try {
            //     await Task.Delay(10000, cancellationToken);
            // } catch (TaskCanceledException) {
            // }
        } while (ShouldStallWrite(jobConfig, path, filesize));
    }
}

} // namespace VodArchiver
