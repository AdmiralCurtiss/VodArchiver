#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "../videoinfo/i-video-info.h"

#include "../job_config.h"
#include "../task_cancellation.h"
#include "../time_types.h"

namespace VodArchiver {
enum class VideoJobStatus {
    NotStarted,
    Running,
    Finished,
    Dead,
};
std::string_view VideoJobStatusToString(VideoJobStatus status);
std::optional<VideoJobStatus> VideoJobStatusFromString(std::string_view sv);

enum class ResultType {
    Failure,
    Success,
    Cancelled,
    Dead,
    UserInputRequired,
    TemporarilyUnavailable,
};
std::string_view ResultTypeToString(ResultType type);
std::optional<ResultType> ResultTypeFromString(std::string_view sv);

struct IUserInputRequest {
    virtual ~IUserInputRequest();
    virtual const std::string& GetQuestion() const = 0;
    virtual const std::vector<std::string>& GetOptions() const = 0;
    virtual void SelectOption(const std::string& option) = 0;
};

struct IVideoJob {
    // To document this somewhere...
    // We have a somewhat complicated access logic for the data stored by jobs. Jobs that are stored
    // in the JobList are accessed by multiple threads, so the access must be synchronized to avoid
    // race conditions. None of this applies to jobs that are not stored in the JobList.
    //
    // All access (both read and write) to the job data *must* be done while holding the
    // JobList::JobsLock. There is one exception to this: The Run() function may be called without
    // holding this lock, it will take care of the locking on its own.

    virtual ~IVideoJob();

    void SetStatus(std::string value);
    std::shared_ptr<IVideoInfo> GetVideoInfo() const;
    void SetVideoInfo(std::shared_ptr<IVideoInfo> videoInfo);
    std::string GetHumanReadableJobName() const;

    virtual bool IsWaitingForUserInput() const = 0;
    virtual std::shared_ptr<IUserInputRequest> GetUserInputRequest() const;
    virtual ResultType Run(JobConfig& jobConfig, TaskCancellation& cancellationToken) = 0;
    virtual std::string GenerateOutputFilename() = 0;

    std::string TextStatus;
    VideoJobStatus JobStatus = VideoJobStatus::NotStarted;
    bool HasBeenValidated = false;
    std::shared_ptr<IVideoInfo> VideoInfo;
    DateTime JobStartTimestamp{.Data = 0};
    DateTime JobFinishTimestamp{.Data = 0};
    std::string Notes;
};

bool ShouldStallWriteRegularFile(JobConfig& jobConfig, std::string_view path, uint64_t filesize);
bool ShouldStallWriteSmallFile(JobConfig& jobConfig, std::string_view path, uint64_t filesize);

void StallWrite(
    JobConfig& jobConfig,
    std::string_view path,
    uint64_t filesize,
    TaskCancellation& cancellationToken,
    const std::function<bool(JobConfig& jobConfig, std::string_view path, uint64_t filesize)>&
        shouldStallWriteCallback,
    const std::function<void(std::string status)>& setStatusCallback);

} // namespace VodArchiver
