#include "time_types.h"

#include <chrono>

namespace VodArchiver {
DateTime DateTime::UtcNow() {
    std::chrono::nanoseconds ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::tai_clock::now().time_since_epoch());
    int64_t ticks = ns.count() / 100;
    static constexpr int64_t EPOCH_DIFFERENCE_IN_TICKS = 617569056000000000;
    return DateTime{.Data = static_cast<uint64_t>(ticks + EPOCH_DIFFERENCE_IN_TICKS)};
}

DateTime DateTime::AddMinutes(int minutes) const {
    int64_t ticks = static_cast<int64_t>(minutes) * static_cast<int64_t>(60u * TICKS_PER_SECOND);
    return DateTime{.Data = this->Data + static_cast<uint64_t>(ticks)};
}
} // namespace VodArchiver
