#pragma once

#include <mutex>

#include "task_cancellation.h"

namespace VodArchiver {
struct DiskMutex;
struct DiskLock {
    DiskLock(const DiskLock& other) = delete;
    DiskLock(DiskLock&& other) = delete;
    DiskLock& operator=(const DiskLock& other) = delete;
    DiskLock& operator=(DiskLock&& other) = delete;
    ~DiskLock();

private:
    DiskLock(std::unique_lock<std::recursive_timed_mutex> lock);

    std::unique_lock<std::recursive_timed_mutex> Lock;

    friend struct DiskMutex;
};
struct DiskMutex {
    DiskLock WaitForFreeSlot(TaskCancellation& cancellationToken);

private:
    std::recursive_timed_mutex Mutex;
};
} // namespace VodArchiver
