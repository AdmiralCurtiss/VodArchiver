#pragma once

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace VodArchiver {
// C# compatible types
static constexpr uint32_t CSHARP_TICKS_PER_SECOND = 10'000'000;

struct DateTime {
    static constexpr uint32_t TICKS_PER_SECOND = CSHARP_TICKS_PER_SECOND;

    uint64_t Data = 0;

    static DateTime FromBinary(int64_t data) {
        if (data < 0) {
            // this means local time instead of UTC so it should do some conversion here, but
            // we'll just ignore this for now, I don't think it actually matters...
        }
        return DateTime{.Data = static_cast<uint64_t>(data)};
    }

    static DateTime FromUnixTime(int64_t timestamp) {
        return FromBinary(621355968000000000).AddSeconds(timestamp);
    }

    static DateTime UtcNow();
    DateTime AddSeconds(int64_t seconds) const;
    DateTime AddMinutes(int minutes) const;

    auto operator<=>(const DateTime& other) const {
        return this->Data <=> other.Data;
    }
};
struct TimeSpan {
    static constexpr uint32_t TICKS_PER_SECOND = CSHARP_TICKS_PER_SECOND;

    int64_t Ticks = 0; // unit is 1e-7 seconds, ie 100 nanoseconds

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

std::string DateTimeToStringForFilesystem(DateTime dt);
std::string DateTimeToStringForGui(DateTime dt);
} // namespace VodArchiver
