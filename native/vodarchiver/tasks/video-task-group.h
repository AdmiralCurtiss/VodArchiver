#pragma once

#include <atomic>
#include <cstdint>
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
    DateTime EarliestPossibleStartTime = DateTime{.Internal = 0};
};

enum class TaskDoneEnum : uint8_t {
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
    VideoTaskGroup(StreamService service,
                   std::function<void()> saveJobsDelegate,
                   std::function<void()> powerEventDelegate,
                   JobConfig* jobConfig,
                   TaskCancellation* cancellationToken);
    ~VideoTaskGroup();

    void Enqueue(IVideoJob* job, bool startImmediately = false);
    bool IsEmpty();
    bool CancelJob(IVideoJob* job);
    bool IsInQueue(IVideoJob* job);
    bool Dequeue(IVideoJob* job);
    void DequeueAll();

    bool IsAutoEnqueue() const {
        return AutoEnqueue.load(std::memory_order_relaxed);
    }

    void SetAutoEnqueue(bool autoEnqueue) {
        AutoEnqueue.store(autoEnqueue, std::memory_order_relaxed);
    }

    StreamService GetService() const {
        return Service;
    }

private:
    void RunJobRunnerThreadFunc();
    void ProcessFinishedTasks();
    IVideoJob* DequeueVideoJobForTask();

    void EnqueueNoLock(IVideoJob* job, bool startImmediately);
    void EnqueueNoLock(std::unique_ptr<WaitingVideoJob> wj);
    bool IsEmptyNoLock();
    bool CancelJobNoLock(IVideoJob* job);
    bool IsJobWaitingNoLock(IVideoJob* job);
    bool IsJobRunningNoLock(IVideoJob* job);
    bool IsJobWaitingOrRunningNoLock(IVideoJob* job);
    bool DequeueNoLock(IVideoJob* job);
    void DequeueAllNoLock();

    StreamService Service = StreamService::Unknown;
    std::atomic<bool> AutoEnqueue = false;

    // This mutex guards access to WaitingJobs and RunningTasks.
    std::mutex JobQueueLock;
    std::vector<std::unique_ptr<WaitingVideoJob>> WaitingJobs;
    std::vector<std::unique_ptr<RunningVideoJob>> RunningTasks;

    size_t MaxJobsRunningPerType = 0;
    JobConfig* JobConf = nullptr;
    TaskCancellation* CancellationToken = nullptr;

    std::function<void()> RequestSaveJobs;
    std::function<void()> RequestPowerEvent;

    std::thread JobRunnerThread;
};
} // namespace VodArchiver
