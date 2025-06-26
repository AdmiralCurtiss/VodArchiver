#pragma once

#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace VodArchiver {
// C# compatible types
struct DateTime {
    uint64_t Data;

    static DateTime FromBinary(int64_t data) {
        if (data < 0) {
            // this means local time instead of UTC so it should do some conversion here, but
            // we'll just ignore this for now, I don't think it actually matters...
        }
        return DateTime{.Data = static_cast<uint64_t>(data)};
    }
};
struct TimeSpan {
    static constexpr uint32_t TICKS_PER_SECOND = 10'000'000;

    int64_t Ticks; // unit is 1e-7 seconds, ie 100 nanoseconds

    static TimeSpan FromSeconds(double seconds) {
        int64_t integer_seconds = static_cast<int64_t>(seconds);
        int64_t sub_integer_ticks = static_cast<int64_t>(
            std::llround(std::fmod(std::abs(seconds) * static_cast<double>(TICKS_PER_SECOND),
                                   static_cast<double>(TICKS_PER_SECOND))));
        return TimeSpan{.Ticks = integer_seconds * static_cast<int64_t>(TICKS_PER_SECOND)
                                 + sub_integer_ticks};
    }
    double GetTotalSeconds() const {
        return static_cast<double>(Ticks) * 1e-7;
    }
};
} // namespace VodArchiver
