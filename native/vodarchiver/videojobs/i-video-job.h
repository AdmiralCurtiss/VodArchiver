#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../statusupdate/i-status-update.h"
#include "../videoinfo/i-video-info.h"

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
    std::shared_ptr<IStatusUpdate> StatusUpdater;

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

    virtual ResultType Run(const std::string& targetFolderPath,
                           const std::string& tempFolderPath,
                           TaskCancellation& cancellationToken) = 0;

protected:
    virtual bool ShouldStallWrite(const std::string& path, uint64_t filesize) const;

public:
    void
        StallWrite(const std::string& path, uint64_t filesize, TaskCancellation& cancellationToken);

    virtual std::string GenerateOutputFilename() = 0;
};
} // namespace VodArchiver
