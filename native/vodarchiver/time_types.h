#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace VodArchiver {
// C# compatible types
static constexpr uint32_t CSHARP_TICKS_PER_SECOND = 10'000'000;

struct DateTime {
    static constexpr uint32_t TICKS_PER_SECOND = CSHARP_TICKS_PER_SECOND;
    static constexpr uint64_t KIND_UNSPECIFIED = 0;
    static constexpr uint64_t KIND_UTC = 1;
    static constexpr uint64_t KIND_LOCAL = 2; // 3 also counts as local

    // In C#, the high two bits are the Kind and the low 62 bits are the Ticks.
    // We flip these in our representation so that sorting doesn't need any special handling.
    uint64_t Internal = 0;

    uint64_t GetTicks() const {
        return ((Internal >> 2) & static_cast<uint64_t>(0x3fff'ffff'ffff'ffffu));
    }

    uint64_t GetKind() const {
        return (Internal & static_cast<uint64_t>(3));
    }

    static DateTime FromTicksAndKind(uint64_t ticks, uint64_t kind) {
        return DateTime{.Internal = (((ticks << 2) & static_cast<uint64_t>(0xffff'ffff'ffff'fffcu))
                                     | (kind & static_cast<uint64_t>(3)))};
    }

    static DateTime FromBinary(int64_t data) {
        // As far as I can tell, kind is supposed to be evaluated when converting to a time?
        // Evaluating it immediately doesn't produce anything sane, at least...
        uint64_t ticks =
            (static_cast<uint64_t>(data) & static_cast<uint64_t>(0x3fff'ffff'ffff'ffffu));
        uint64_t kind = ((static_cast<uint64_t>(data) >> 62) & static_cast<uint64_t>(3));
        return DateTime::FromTicksAndKind(ticks, kind);
    }

    int64_t ToBinary() const {
        uint64_t time = GetTicks();
        uint64_t kind = GetKind();
        uint64_t flipped = time | (kind << 62);
        return static_cast<int64_t>(flipped);
    }

    static DateTime FromUnixTime(int64_t timestamp) {
        return FromBinary(621355968000000000).AddSeconds(timestamp);
    }
    int64_t ToUnixTime() const {
        return static_cast<int64_t>(GetTicks() - 621355968000000000u) / DateTime::TICKS_PER_SECOND;
    }

    static DateTime FromDate(uint64_t year,
                             uint64_t month,
                             uint64_t day,
                             uint64_t hours,
                             uint64_t minutes,
                             uint64_t seconds);

    static DateTime UtcNow();
    DateTime AddTicks(int64_t ticks) const;
    DateTime AddSeconds(int64_t seconds) const;
    DateTime AddMinutes(int minutes) const;
    DateTime AddHours(int hours) const;

    constexpr auto operator<=>(const DateTime& other) const = default;
};

struct TimeSpan {
    static constexpr uint32_t TICKS_PER_SECOND = CSHARP_TICKS_PER_SECOND;

    int64_t Ticks = 0; // unit is 1e-7 seconds, ie 100 nanoseconds

    static TimeSpan FromIntegerSeconds(int64_t seconds);
    static TimeSpan FromDoubleSeconds(double seconds);
    static std::optional<TimeSpan> ParseFromSeconds(std::string_view value);
    double GetTotalSeconds() const {
        return static_cast<double>(Ticks) * 1e-7;
    }

    constexpr TimeSpan operator+(const TimeSpan& other) const {
        return TimeSpan{.Ticks = this->Ticks + other.Ticks};
    }
    constexpr TimeSpan operator-(const TimeSpan& other) const {
        return TimeSpan{.Ticks = this->Ticks - other.Ticks};
    }
    constexpr auto operator<=>(const TimeSpan& other) const = default;
};

std::string DateTimeToStringForFilesystem(DateTime dt);
std::string_view DateTimeToStringForGui(DateTime dt, std::array<char, 24>& buffer);
std::string DateToString(DateTime dt);

std::string_view TimeSpanToStringForGui(TimeSpan ts, std::array<char, 24>& buffer);
std::string TimeSpanToTotalSecondsString(const TimeSpan& timeSpan);
} // namespace VodArchiver
