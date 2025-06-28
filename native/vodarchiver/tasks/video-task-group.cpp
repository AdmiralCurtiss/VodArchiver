#include "video-task-group.h"

#include <chrono>
#include <format>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>

#include "util/scope.h"

#include "../time_types.h"
#include "../videojobs/i-video-job.h"

namespace VodArchiver {
static bool IsAllowedToStart(const WaitingVideoJob& wvj) {
    return wvj.Job != nullptr && DateTime::UtcNow() >= wvj.EarliestPossibleStartTime
           && !wvj.Job->IsWaitingForUserInput();
}

static size_t GetDefaultMaxJobs(StreamService service) {
    switch (service) {
        case StreamService::Youtube: return 1;
        case StreamService::RawUrl: return 1;
        case StreamService::FFMpegJob: return 1;
        case StreamService::Twitch: return 3;
        case StreamService::TwitchChatReplay: return 1;
        default: return 1;
    }
}

VideoTaskGroup::VideoTaskGroup(StreamService service,
                               std::function<void()> saveJobsDelegate,
                               std::function<void()> powerEventDelegate,
                               TaskCancellation* cancellationToken)
  : Service(service)
  , MaxJobsRunningPerType(GetDefaultMaxJobs(service))
  , CancellationToken(cancellationToken)
  , RequestSaveJobs(saveJobsDelegate)
  , RequestPowerEvent(powerEventDelegate) {
    JobRunnerThread = std::thread(std::bind(&VideoTaskGroup::RunJobRunnerThreadFunc, this));
}

VideoTaskGroup::~VideoTaskGroup() {
    {
        std::lock_guard lock(JobQueueLock);
        DequeueAll();
        CancellationToken->CancelTask();
    }
    WaitForJobRunnerThreadToEnd();
}

void VideoTaskGroup::RunJobRunnerThreadFunc() {
    while (true) {
        if (CancellationToken->IsCancellationRequested()) {
            std::lock_guard lock(JobQueueLock);
            if (RunningTasks.empty()) {
                break;
            }
        }

        try {
            // FIXME: This is could be much better implemented with a condition_variable...
            using namespace std::chrono_literals;
            if (!CancellationToken->IsCancellationRequested()) {
                std::this_thread::sleep_for(750ms);
            } else {
                std::this_thread::sleep_for(50ms);
            }
        } catch (...) {
        }

        try {
            if (!CancellationToken->IsCancellationRequested()) {
                std::lock_guard lock(JobQueueLock);

                // find jobs we can start
                IVideoJob* job = nullptr;
                while ((job = DequeueVideoJobForTask()) != nullptr) {
                    // and run them
                    auto rvj = std::make_unique<RunningVideoJob>();
                    rvj->Job = job;
                    rvj->Task =
                        std::thread(std::bind(&VideoTaskGroup::RunJobThreadFunc, this, rvj.get()));
                    RunningTasks.emplace_back(std::move(rvj));
                }
            } else {
                std::lock_guard lock(JobQueueLock);

                // stop all running tasks
                for (auto& t : RunningTasks) {
                    t->CancellationToken.CancelTask();
                }
            }
        } catch (...) {
        }

        // TODO: murder tasks that are unresponsive

        try {
            std::lock_guard lock(JobQueueLock);
            ProcessFinishedTasks();
        } catch (...) {
        }
    }
}

void VideoTaskGroup::ProcessFinishedTasks() {
    std::lock_guard lock(JobQueueLock);
    for (size_t i = RunningTasks.size(); i > 0; --i) {
        if (RunningTasks[i - 1]->Done.load() != TaskDoneEnum::NotDone) {
            // need to remove the task first, as otherwise the Add() still detects it as running
            std::unique_ptr<RunningVideoJob> task = std::move(RunningTasks[i - 1]);
            RunningTasks.erase(RunningTasks.begin() + (i - 1));

            if (task->Done.load() == TaskDoneEnum::FinishedNormally) {
                ResultType result = task->Result.load();
                if (result == ResultType::TemporarilyUnavailable) {
                    auto wvj = std::make_unique<WaitingVideoJob>();
                    wvj->Job = task->Job;
                    wvj->EarliestPossibleStartTime = DateTime::UtcNow().AddMinutes(30);
                    Add(std::move(wvj));
                }
            } else {
                if (!task->ErrorString.empty()) {
                    task->Job->SetStatus("Failed via unexpected exception: " + task->ErrorString);
                } else {
                    task->Job->SetStatus("Failed for unknown reasons.");
                }
            }

            RequestSaveJobs();
            RequestPowerEvent();
        }
    }
}

void VideoTaskGroup::RunJobThreadFunc(RunningVideoJob* rvj) {
    if (rvj == nullptr) {
        return;
    }

    rvj->Done.store(TaskDoneEnum::NotDone);
    TaskDoneEnum taskDoneEnum = TaskDoneEnum::FinishedWithError;
    auto scope = HyoutaUtils::MakeScopeGuard([&] { rvj->Done.store(taskDoneEnum); });
    if (rvj->Job != nullptr) {
        IVideoJob& job = *rvj->Job;
        bool wasDead = job.JobStatus == VideoJobStatus::Dead;
        try {
            if (job.JobStatus != VideoJobStatus::Finished) {
                job.JobStartTimestamp = DateTime::UtcNow();
                ResultType result = job.Run(rvj->CancellationToken);
                if (result == ResultType::Success) {
                    job.JobFinishTimestamp = DateTime::UtcNow();
                    job.JobStatus = VideoJobStatus::Finished;
                } else if (result == ResultType::Dead) {
                    job.SetStatus("Dead.");
                    job.JobStatus = VideoJobStatus::Dead;
                } else {
                    job.JobStatus = wasDead ? VideoJobStatus::Dead : VideoJobStatus::NotStarted;
                }

                if (result == ResultType::Cancelled) {
                    job.SetStatus("Cancelled during: " + job.GetStatus());
                }

                if (result == ResultType::TemporarilyUnavailable) {
                    job.SetStatus("Temporarily unavailable, retrying later.");
                }

                rvj->Result.store(result);
                taskDoneEnum = TaskDoneEnum::FinishedNormally;
            } else {
                // task is already finished, no need to do anything
                rvj->Result.store(ResultType::Success);
                taskDoneEnum = TaskDoneEnum::FinishedNormally;
            }
        } catch (const std::exception& ex) {
            job.JobStatus = wasDead ? VideoJobStatus::Dead : VideoJobStatus::NotStarted;
            job.SetStatus(std::format("ERROR: {}", ex.what()));
            rvj->Result.store(ResultType::Failure);
            taskDoneEnum = TaskDoneEnum::FinishedNormally;
        } catch (...) {
            job.JobStatus = wasDead ? VideoJobStatus::Dead : VideoJobStatus::NotStarted;
            job.SetStatus("UNKNOWN ERROR");
            rvj->Result.store(ResultType::Failure);
            taskDoneEnum = TaskDoneEnum::FinishedNormally;
        }
    } else {
        rvj->Result.store(ResultType::Failure);
        taskDoneEnum = TaskDoneEnum::FinishedNormally;
    }
}

IVideoJob* VideoTaskGroup::DequeueVideoJobForTask() {
    std::lock_guard lock(JobQueueLock);
    bool found = false;
    size_t waitingJobIndex = 0;
    for (size_t i = 0; i < WaitingJobs.size(); ++i) {
        auto& wj = *WaitingJobs[i];
        if (wj.StartImmediately
            || (RunningTasks.size() < MaxJobsRunningPerType && IsAllowedToStart(wj))) {
            waitingJobIndex = i;
            found = true;
            break;
        }
    }

    if (found) {
        IVideoJob* job = WaitingJobs[waitingJobIndex]->Job;
        WaitingJobs.erase(WaitingJobs.begin() + waitingJobIndex);
        return job;
    }

    return nullptr;
}

void VideoTaskGroup::Add(std::unique_ptr<WaitingVideoJob> wj) {
    std::lock_guard lock(JobQueueLock);
    if (!CancellationToken->IsCancellationRequested()) {
        if (IsJobWaiting(wj->Job)) {
            for (auto& alreadyEnqueuedJob : WaitingJobs) {
                if (alreadyEnqueuedJob->Job == wj->Job) {
                    alreadyEnqueuedJob->EarliestPossibleStartTime = wj->EarliestPossibleStartTime;
                    alreadyEnqueuedJob->StartImmediately = wj->StartImmediately;
                }
            }
        } else if (!IsJobRunning(wj->Job)) {
            WaitingJobs.emplace_back(std::move(wj));
        }
    }
}

bool VideoTaskGroup::IsEmpty() {
    std::lock_guard lock(JobQueueLock);
    return WaitingJobs.empty() && RunningTasks.empty();
}

bool VideoTaskGroup::IsJobWaiting(IVideoJob* job) {
    std::lock_guard lock(JobQueueLock);
    for (auto& wj : WaitingJobs) {
        if (wj->Job == job) {
            return true;
        }
    }
    return false;
}

bool VideoTaskGroup::IsJobRunning(IVideoJob* job) {
    std::lock_guard lock(JobQueueLock);
    for (auto& rt : RunningTasks) {
        if (rt->Job == job) {
            return true;
        }
    }
    return false;
}

bool VideoTaskGroup::IsJobWaitingOrRunning(IVideoJob* job) {
    std::lock_guard lock(JobQueueLock);
    return IsJobWaiting(job) || IsJobRunning(job);
}

bool VideoTaskGroup::CancelJob(IVideoJob* job) {
    std::lock_guard lock(JobQueueLock);
    bool cancelled = false;
    for (auto& rt : RunningTasks) {
        if (rt->Job == job) {
            rt->CancellationToken.CancelTask();
            cancelled = true;
        }
    }
    return cancelled;
}

bool VideoTaskGroup::IsInQueue(IVideoJob* job) {
    std::lock_guard lock(JobQueueLock);
    return IsJobWaiting(job);
}

bool VideoTaskGroup::Dequeue(IVideoJob* job) {
    std::lock_guard lock(JobQueueLock);
    bool found = false;
    for (auto it = WaitingJobs.begin(); it != WaitingJobs.end();) {
        if ((*it)->Job == job) {
            it = WaitingJobs.erase(it);
            found = true;
        } else {
            ++it;
        }
    }
    return found;
}

void VideoTaskGroup::DequeueAll() {
    std::lock_guard lock(JobQueueLock);
    WaitingJobs.clear();
}

void VideoTaskGroup::WaitForJobRunnerThreadToEnd() {
    if (JobRunnerThread.joinable()) {
        JobRunnerThread.join();
    }
}
} // namespace VodArchiver
