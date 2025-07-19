#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "../job_config.h"
#include "../task_cancellation.h"
#include "../time_types.h"
#include "../videojobs/i-video-job.h"

namespace VodArchiver {
struct WaitingVideoJob {
    IVideoJob* Job = nullptr;
    bool StartImmediately = false;
    DateTime EarliestPossibleStartTime = DateTime{.Data = 0};
};

enum class TaskDoneEnum {
    NotDone,
    FinishedNormally,
    FinishedWithError,
};

struct RunningVideoJob {
    IVideoJob* Job = nullptr;
    JobConfig* JobConf = nullptr;
    TaskCancellation CancellationToken;
    std::thread Task;
    std::atomic<TaskDoneEnum> Done = TaskDoneEnum::NotDone;
    std::atomic<ResultType> Result = ResultType::Failure;
    std::string ErrorString;
};

struct VideoTaskGroup {
    StreamService Service = StreamService::Unknown;
    std::vector<std::unique_ptr<WaitingVideoJob>> WaitingJobs;
    std::recursive_mutex JobQueueLock;
    size_t MaxJobsRunningPerType = 0;
    JobConfig* JobConf = nullptr;
    TaskCancellation* CancellationToken = nullptr;

    std::thread JobRunnerThread;
    std::vector<std::unique_ptr<RunningVideoJob>> RunningTasks;

    std::function<void()> RequestSaveJobs;
    std::function<void()> RequestPowerEvent;

    bool AutoEnqueue = false;

    VideoTaskGroup(StreamService service,
                   std::function<void()> saveJobsDelegate,
                   std::function<void()> powerEventDelegate,
                   JobConfig* jobConfig,
                   TaskCancellation* cancellationToken);
    ~VideoTaskGroup();

    void Add(IVideoJob* job);
    void Add(std::unique_ptr<WaitingVideoJob> wj);

    bool IsEmpty();
    bool CancelJob(IVideoJob* job);
    bool IsInQueue(IVideoJob* job);
    bool Dequeue(IVideoJob* job);
    void DequeueAll();

    void WaitForJobRunnerThreadToEnd();

private:
    void RunJobRunnerThreadFunc();
    void ProcessFinishedTasks();
    void RunJobThreadFunc(RunningVideoJob* rvj);
    IVideoJob* DequeueVideoJobForTask();

    bool IsJobWaiting(IVideoJob* job);
    bool IsJobRunning(IVideoJob* job);
    bool IsJobWaitingOrRunning(IVideoJob* job);
};
} // namespace VodArchiver
