#include "task_cancellation.h"

#include <atomic>
#include <chrono>
#include <mutex>

namespace VodArchiver {
TaskCancellation::TaskCancellation() = default;
TaskCancellation::~TaskCancellation() = default;

bool TaskCancellation::IsCancellationRequested() {
    std::lock_guard lock(Mutex);
    return CancellationRequested.load(std::memory_order_relaxed);
}

void TaskCancellation::CancelTask() {
    std::lock_guard lock(Mutex);
    CancellationRequested.store(true, std::memory_order_relaxed);
    CancellationCondVar.notify_all();
}

void TaskCancellation::Reset() {
    std::lock_guard lock(Mutex);
    CancellationRequested.store(false, std::memory_order_relaxed);
}

bool TaskCancellation::DelayFor(std::chrono::nanoseconds ns) {
    auto when = std::chrono::steady_clock::now() + ns;
    std::unique_lock lock(Mutex);
    return !CancellationCondVar.wait_until(
        lock, when, [&] { return CancellationRequested.load(std::memory_order_relaxed); });
}
} // namespace VodArchiver
