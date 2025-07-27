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
    virtual ~IVideoJob();

public: // should be private
    std::string _Status;

public:
    virtual const std::string& GetStatus() const;
    virtual void SetStatus(std::string value);

    virtual bool IsWaitingForUserInput() const = 0;

    virtual std::shared_ptr<IUserInputRequest> GetUserInputRequest() const;

    virtual std::string GetHumanReadableJobName() const;

public:
    size_t Index = 0; // FIXME: Is this even used for anything?

    VideoJobStatus JobStatus = VideoJobStatus::NotStarted;

public: // should be private
    std::shared_ptr<IVideoInfo> _VideoInfo;

public:
    virtual std::shared_ptr<IVideoInfo> GetVideoInfo() const;
    virtual void SetVideoInfo(std::shared_ptr<IVideoInfo> videoInfo);

    bool HasBeenValidated = false;

    std::string Notes;

    DateTime JobStartTimestamp{.Data = 0};

    DateTime JobFinishTimestamp{.Data = 0};

    virtual ResultType Run(JobConfig& jobConfig, TaskCancellation& cancellationToken) = 0;

    virtual std::string GenerateOutputFilename() = 0;
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
