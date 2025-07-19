#include "disk_lock.h"

#include <chrono>
#include <mutex>
#include <utility>

#include "task_cancellation.h"

namespace VodArchiver {
DiskLock::DiskLock(std::unique_lock<std::recursive_timed_mutex> lock) : Lock(std::move(lock)) {}

DiskLock::~DiskLock() = default;

DiskLock DiskMutex::WaitForFreeSlot(TaskCancellation& cancellationToken) {
    // TODO: Is there a better way to do this? Waiting for a mutex to become available but observing
    // a cancellation signal?
    std::unique_lock<std::recursive_timed_mutex> lock(Mutex, std::defer_lock);
    while (true) {
        if (lock.try_lock_for(std::chrono::milliseconds(500))) {
            return DiskLock(std::move(lock));
        }
        if (cancellationToken.IsCancellationRequested()) {
            return DiskLock(std::unique_lock<std::recursive_timed_mutex>());
        }
    }
}
} // namespace VodArchiver
