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

// There's multiple threads involved here.
//
// On VideoTaskGroup construction, the JobRunnerThread is initialized and runs the
// RunJobRunnerThreadFunc in a loop until the VideoTaskGroup is destructed again.
// The JobRunnerThread may spawn an arbitrary amount of worker threads (stored as RunningVideoJob
// objects in the RunningTasks vector) that will each run the RunJobThreadFunc.
//
// The worker threads are rather uninteresting. They only have access to the single job and the
// locking constructs accessible through JobConfig and shouldn't cause any problems.
//
// The JobRunnerThread is somewhat tricky, since at any point some other thread may query job
// states, enqueue new jobs, dequeue jobs, etc. So access to the data structures is synchronized
// through the JobQueueLock. This needs to be safely entangled with the JobsLock that guards access
// to the job data in the JobsList! In particular, the external caller may or may not already hold
// the JobsLock when calling a function to access the job queue.

namespace VodArchiver {
// This is the function called when a job is executed.
static void RunJobThreadFunc(RunningVideoJob* rvj) {
    if (rvj == nullptr) {
        return;
    }

    rvj->Done.store(TaskDoneEnum::NotDone);
    TaskDoneEnum taskDoneEnum = TaskDoneEnum::FinishedWithError;
    auto scope = HyoutaUtils::MakeScopeGuard([&] { rvj->Done.store(taskDoneEnum); });
    if (rvj->Job != nullptr) {
        std::unique_lock lock(*rvj->JobConf->JobsLock);
        IVideoJob& job = *rvj->Job;
        bool wasDead = job.JobStatus == VideoJobStatus::Dead;
        try {
            if (job.JobStatus != VideoJobStatus::Finished) {
                job.JobStartTimestamp = DateTime::UtcNow();
                ResultType result = ResultType::Failure;
                {
                    lock.unlock();
                    result = job.Run(*rvj->JobConf, rvj->CancellationToken);
                    lock.lock();
                }
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
                    job.SetStatus("Cancelled during: " + job.TextStatus);
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
                               JobConfig* jobConfig,
                               TaskCancellation* cancellationToken)
  : Service(service)
  , MaxJobsRunningPerType(GetDefaultMaxJobs(service))
  , JobConf(jobConfig)
  , CancellationToken(cancellationToken)
  , RequestSaveJobs(std::move(saveJobsDelegate))
  , RequestPowerEvent(std::move(powerEventDelegate)) {
    JobRunnerThread = std::thread(std::bind(&VideoTaskGroup::RunJobRunnerThreadFunc, this));
}

VideoTaskGroup::~VideoTaskGroup() {
    {
        std::lock_guard lock(JobQueueLock);
        DequeueAllNoLock();              // dequeue all waiting jobs
        CancellationToken->CancelTask(); // tell job runner thread that it should finish
    }

    // wait for job runner thread to end
    if (JobRunnerThread.joinable()) {
        JobRunnerThread.join();
    }
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
                // find jobs we can start
                IVideoJob* job = nullptr;
                while ((job = DequeueVideoJobForTask()) != nullptr) {
                    // and run them
                    auto rvj = std::make_unique<RunningVideoJob>();
                    rvj->Job = job;
                    rvj->JobConf = this->JobConf;
                    rvj->Task = std::thread(RunJobThreadFunc, rvj.get());

                    {
                        std::lock_guard lock(JobQueueLock);
                        RunningTasks.emplace_back(std::move(rvj));
                    }
                }
            } else {
                std::lock_guard lock(JobQueueLock);

                // tell all running tasks that they should finish ASAP
                for (auto& t : RunningTasks) {
                    t->CancellationToken.CancelTask();
                }
            }
        } catch (...) {
        }

        // TODO: murder tasks that are unresponsive

        try {
            ProcessFinishedTasks();
        } catch (...) {
        }
    }
}

void VideoTaskGroup::ProcessFinishedTasks() {
    while (true) {
        bool removedTask = false;
        std::unique_lock lock(JobQueueLock);
        for (auto it = RunningTasks.begin(); it != RunningTasks.end(); ++it) {
            if ((*it)->Done.load() != TaskDoneEnum::NotDone) {
                // remove the task
                std::unique_ptr<RunningVideoJob> task = std::move(*it);
                RunningTasks.erase(it);
                lock.unlock(); // unlock the queue to avoid deadlock with JobsLock
                removedTask = true;

                task->Task.join();
                if (task->Done.load() == TaskDoneEnum::FinishedNormally) {
                    ResultType result = task->Result.load();
                    if (result == ResultType::TemporarilyUnavailable) {
                        // re-enqueue with a future start time
                        auto wvj = std::make_unique<WaitingVideoJob>();
                        wvj->Job = task->Job;
                        wvj->EarliestPossibleStartTime = DateTime::UtcNow().AddMinutes(30);

                        std::lock_guard lock2(JobQueueLock);
                        EnqueueNoLock(std::move(wvj));
                    }
                } else {
                    std::lock_guard lock2(*JobConf->JobsLock);
                    if (!task->ErrorString.empty()) {
                        task->Job->SetStatus("Failed via unexpected exception: "
                                             + task->ErrorString);
                    } else {
                        task->Job->SetStatus("Failed for unknown reasons.");
                    }
                }

                RequestSaveJobs();
                RequestPowerEvent();

                // break out of the for (RunningTasks) loop and re-lock the queue lock
                break;
            }
        }

        if (!removedTask) {
            // no task to remove found, we're done
            return;
        }
    }
}

IVideoJob* VideoTaskGroup::DequeueVideoJobForTask() {
    // JobsLock must be locked first to avoid deadlock!
    std::lock_guard lock2(*JobConf->JobsLock);
    {
        std::lock_guard lock(JobQueueLock);
        bool found = false;
        size_t waitingJobIndex = 0;
        {
            const auto IsAllowedToStart = [](const WaitingVideoJob& wvj) -> bool {
                return wvj.Job != nullptr && DateTime::UtcNow() >= wvj.EarliestPossibleStartTime
                       && !wvj.Job->IsWaitingForUserInput();
            };
            for (size_t i = 0; i < WaitingJobs.size(); ++i) {
                auto& wj = *WaitingJobs[i];
                if (wj.StartImmediately
                    || (RunningTasks.size() < MaxJobsRunningPerType && IsAllowedToStart(wj))) {
                    waitingJobIndex = i;
                    found = true;
                    break;
                }
            }
        }

        if (found) {
            IVideoJob* job = WaitingJobs[waitingJobIndex]->Job;
            WaitingJobs.erase(WaitingJobs.begin() + waitingJobIndex);
            return job;
        }
    }

    return nullptr;
}

void VideoTaskGroup::Enqueue(IVideoJob* job, bool startImmediately) {
    std::lock_guard lock(JobQueueLock);
    EnqueueNoLock(job, startImmediately);
}

void VideoTaskGroup::EnqueueNoLock(IVideoJob* job, bool startImmediately) {
    auto wj = std::make_unique<WaitingVideoJob>();
    wj->Job = job;
    wj->StartImmediately = startImmediately;
    EnqueueNoLock(std::move(wj));
}

void VideoTaskGroup::EnqueueNoLock(std::unique_ptr<WaitingVideoJob> wj) {
    if (!CancellationToken->IsCancellationRequested()) {
        if (IsJobWaitingNoLock(wj->Job)) {
            for (auto& alreadyEnqueuedJob : WaitingJobs) {
                if (alreadyEnqueuedJob->Job == wj->Job) {
                    alreadyEnqueuedJob->EarliestPossibleStartTime = wj->EarliestPossibleStartTime;
                    alreadyEnqueuedJob->StartImmediately = wj->StartImmediately;
                }
            }
        } else if (!IsJobRunningNoLock(wj->Job)) {
            WaitingJobs.emplace_back(std::move(wj));
        }
    }
}

bool VideoTaskGroup::IsEmpty() {
    std::lock_guard lock(JobQueueLock);
    return IsEmptyNoLock();
}

bool VideoTaskGroup::IsEmptyNoLock() {
    return WaitingJobs.empty() && RunningTasks.empty();
}

bool VideoTaskGroup::IsJobWaitingNoLock(IVideoJob* job) {
    for (auto& wj : WaitingJobs) {
        if (wj->Job == job) {
            return true;
        }
    }
    return false;
}

bool VideoTaskGroup::IsJobRunningNoLock(IVideoJob* job) {
    for (auto& rt : RunningTasks) {
        if (rt->Job == job) {
            return true;
        }
    }
    return false;
}

bool VideoTaskGroup::IsJobWaitingOrRunningNoLock(IVideoJob* job) {
    return IsJobWaitingNoLock(job) || IsJobRunningNoLock(job);
}

bool VideoTaskGroup::CancelJob(IVideoJob* job) {
    std::lock_guard lock(JobQueueLock);
    return CancelJobNoLock(job);
}

bool VideoTaskGroup::CancelJobNoLock(IVideoJob* job) {
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
    return IsJobWaitingNoLock(job);
}

bool VideoTaskGroup::Dequeue(IVideoJob* job) {
    std::lock_guard lock(JobQueueLock);
    return DequeueNoLock(job);
}

bool VideoTaskGroup::DequeueNoLock(IVideoJob* job) {
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
    DequeueAllNoLock();
}

void VideoTaskGroup::DequeueAllNoLock() {
    WaitingJobs.clear();
}
} // namespace VodArchiver
