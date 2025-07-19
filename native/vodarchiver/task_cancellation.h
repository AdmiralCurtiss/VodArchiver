#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace VodArchiver {
struct TaskCancellation {
    TaskCancellation();
    TaskCancellation(const TaskCancellation& other) = delete;
    TaskCancellation(TaskCancellation&& other) = delete;
    TaskCancellation& operator=(const TaskCancellation& other) = delete;
    TaskCancellation& operator=(TaskCancellation&& other) = delete;
    ~TaskCancellation();

    bool IsCancellationRequested();
    void CancelTask();
    void Reset();

    // returns true if wait finished normally, false if interrupted by cancellation
    bool DelayFor(std::chrono::nanoseconds ns);

private:
    std::mutex Mutex;
    std::condition_variable CancellationCondVar;
    std::atomic<bool> CancellationRequested = false;
};
} // namespace VodArchiver
