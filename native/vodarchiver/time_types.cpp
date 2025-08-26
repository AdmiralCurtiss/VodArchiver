#include "time_types.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <limits>

#include "util/number.h"

#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(_mul128)
#endif

namespace VodArchiver {
DateTime DateTime::UtcNow() {
    std::chrono::nanoseconds ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::tai_clock::now().time_since_epoch());
    int64_t ticks = ns.count() / 100;
    static constexpr int64_t EPOCH_DIFFERENCE_IN_TICKS = 617569056000000000;
    return DateTime{.Data = static_cast<uint64_t>(ticks + EPOCH_DIFFERENCE_IN_TICKS)};
}

DateTime DateTime::FromDate(uint64_t year,
                            uint64_t month,
                            uint64_t day,
                            uint64_t hours,
                            uint64_t minutes,
                            uint64_t seconds) {
    const std::chrono::year_month_day ymd(std::chrono::year(static_cast<int>(year)),
                                          std::chrono::month(static_cast<unsigned int>(month)),
                                          std::chrono::day(static_cast<unsigned int>(day)));
    static constexpr std::chrono::year_month_day unixstamp(
        std::chrono::year(1970), std::chrono::month(1), std::chrono::day(1));
    const auto days = (std::chrono::sys_days(ymd) - std::chrono::sys_days(unixstamp)).count();
    static constexpr int SECONDS_PER_MINUTE = 60;
    static constexpr int SECONDS_PER_HOUR = 60 * 60;
    static constexpr int SECONDS_PER_DAY = 24 * 60 * 60;
    return DateTime::FromUnixTime(static_cast<int64_t>(days) * SECONDS_PER_DAY
                                  + static_cast<int64_t>(hours) * SECONDS_PER_HOUR
                                  + static_cast<int64_t>(minutes) * SECONDS_PER_MINUTE
                                  + static_cast<int64_t>(seconds));
}

DateTime DateTime::AddSeconds(int64_t seconds) const {
    int64_t ticks = static_cast<int64_t>(seconds) * static_cast<int64_t>(TICKS_PER_SECOND);
    return DateTime{.Data = this->Data + static_cast<uint64_t>(ticks)};
}

DateTime DateTime::AddMinutes(int minutes) const {
    int64_t ticks = static_cast<int64_t>(minutes) * static_cast<int64_t>(60u * TICKS_PER_SECOND);
    return DateTime{.Data = this->Data + static_cast<uint64_t>(ticks)};
}

DateTime DateTime::AddHours(int hours) const {
    int64_t ticks = static_cast<int64_t>(hours)
                    * static_cast<int64_t>(60u * 60u * static_cast<int64_t>(TICKS_PER_SECOND));
    return DateTime{.Data = this->Data + static_cast<uint64_t>(ticks)};
}

TimeSpan TimeSpan::FromSecondsAndSubsecondTicks(int64_t seconds, int64_t subseconds) {
#ifdef _MSC_VER
    int64_t high;
    int64_t ticks = _mul128(seconds, static_cast<int64_t>(TimeSpan::TICKS_PER_SECOND), &high);
    bool overflow = (high != 0);
#else
    int64_t ticks;
    bool overflow =
        __builtin_mul_overflow(seconds, static_cast<int64_t>(TimeSpan::TICKS_PER_SECOND), &ticks);
#endif
    if (overflow) {
        // overflow in the multiply, return max value
        if (seconds < 0) {
            return TimeSpan{.Ticks = std::numeric_limits<int64_t>::min()};
        } else {
            return TimeSpan{.Ticks = std::numeric_limits<int64_t>::max()};
        }
    }
    if (ticks < 0) {
        if (ticks < std::numeric_limits<int64_t>::min() + subseconds) {
            return TimeSpan{.Ticks = std::numeric_limits<int64_t>::min()};
        }
        return TimeSpan{.Ticks = ticks - subseconds};
    } else {
        if (ticks > std::numeric_limits<int64_t>::max() - subseconds) {
            return TimeSpan{.Ticks = std::numeric_limits<int64_t>::max()};
        }
        return TimeSpan{.Ticks = ticks + subseconds};
    }
}

std::optional<TimeSpan> TimeSpan::ParseFromSeconds(std::string_view value) {
    if (auto dotpos = value.find('.'); dotpos != std::string_view::npos) {
        // try to parse the seconds and subsections separately so we don't lose information from the
        // double conversion
        std::string_view left = value.substr(0, dotpos);
        std::string_view right = value.substr(dotpos + 1);
        if (right.size() == 0) {
            if (auto sec = HyoutaUtils::NumberUtils::ParseInt64(left)) {
                return TimeSpan::FromSecondsAndSubsecondTicks(*sec, 0);
            }
        } else if (right.size() > 0 && right.size() <= 7) {
            if (auto sec = HyoutaUtils::NumberUtils::ParseInt64(left)) {
                if (auto subsec = HyoutaUtils::NumberUtils::ParseUInt64(right)) {
                    for (size_t i = 0; i < (7 - right.size()); ++i) {
                        *subsec *= 10;
                    }
                    return TimeSpan::FromSecondsAndSubsecondTicks(*sec, *subsec);
                }
            }
        }
    }
    if (auto l = HyoutaUtils::NumberUtils::ParseInt64(value)) {
        return TimeSpan::FromSecondsAndSubsecondTicks(*l, 0);
    }
    if (auto l = HyoutaUtils::NumberUtils::ParseDouble(value)) {
        return TimeSpan::FromSeconds(*l);
    }
    return std::nullopt;
}

std::string DateTimeToStringForFilesystem(DateTime dt) {
    static constexpr int64_t EPOCH_DIFFERENCE_IN_TICKS = 617569056000000000;
    uint64_t seconds = (dt.Data / DateTime::TICKS_PER_SECOND) % 60;
    std::chrono::nanoseconds ns((dt.Data - EPOCH_DIFFERENCE_IN_TICKS) * 100);
    std::chrono::time_point<std::chrono::tai_clock> epoch;
    auto target = (epoch + ns);
    return std::format("{0:%Y}-{0:%m}-{0:%d}_{0:%H}-{0:%M}-{1:02}", target, seconds);
}

std::string_view DateTimeToStringForGui(DateTime dt, std::array<char, 24>& buffer) {
    static constexpr int64_t EPOCH_DIFFERENCE_IN_TICKS = 617569056000000000;
    uint64_t seconds = (dt.Data / DateTime::TICKS_PER_SECOND) % 60;
    std::chrono::nanoseconds ns((dt.Data - EPOCH_DIFFERENCE_IN_TICKS) * 100);
    std::chrono::time_point<std::chrono::tai_clock> epoch;
    auto target = (epoch + ns);
    auto result = std::format_to_n(buffer.data(),
                                   buffer.size() - 1,
                                   "{0:%Y}-{0:%m}-{0:%d} {0:%H}:{0:%M}:{1:02}",
                                   target,
                                   seconds);
    *result.out = '\0';
    return std::string_view(buffer.data(), result.out);
}

std::string DateToString(DateTime dt) {
    static constexpr int64_t EPOCH_DIFFERENCE_IN_TICKS = 617569056000000000;
    std::chrono::nanoseconds ns((dt.Data - EPOCH_DIFFERENCE_IN_TICKS) * 100);
    std::chrono::time_point<std::chrono::tai_clock> epoch;
    auto target = (epoch + ns);
    return std::format("{0:%Y}-{0:%m}-{0:%d}", target);
}

std::string DateTimeToBinaryString(const DateTime& dt) {
    return std::format("{}", static_cast<int64_t>(dt.Data));
}

std::string_view TimeSpanToStringForGui(TimeSpan ts, std::array<char, 24>& buffer) {
    bool isNegative = ts.Ticks < 0;
    uint64_t absoluteTicks =
        (isNegative ? -static_cast<uint64_t>(ts.Ticks) : static_cast<uint64_t>(ts.Ticks));
    uint64_t seconds = (ts.Ticks / TimeSpan::TICKS_PER_SECOND);
    uint64_t subsecondTicks = (ts.Ticks % TimeSpan::TICKS_PER_SECOND);
    uint64_t minutes = (seconds / 60);
    uint64_t hours = (minutes / 60);
    auto result = std::format_to_n(buffer.data(),
                                   buffer.size() - 1,
                                   "{}{}:{:02}:{:02}",
                                   isNegative ? "-" : "",
                                   hours,
                                   minutes % 60,
                                   seconds % 60);
    if (subsecondTicks != 0) {
        result = std::format_to_n(result.out,
                                  (buffer.size() - 1) - (result.out - buffer.data()),
                                  ".{:07}",
                                  subsecondTicks);
        std::string_view sv(buffer.data(), result.out);
        while (sv.ends_with('0')) {
            sv = sv.substr(0, sv.size() - 1);
        }
        buffer[sv.size()] = '\0';
        return sv;
    }
    *result.out = '\0';
    return std::string_view(buffer.data(), result.out);
}

std::string TimeSpanToTotalSecondsString(const TimeSpan& timeSpan) {
    int64_t ticks = timeSpan.Ticks;
    uint64_t t;
    bool neg;
    if (ticks >= 0) {
        t = static_cast<uint64_t>(ticks);
        neg = false;
    } else if (ticks == std::numeric_limits<int64_t>::min()) {
        t = static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + static_cast<uint64_t>(1);
        neg = true;
    } else {
        t = static_cast<uint64_t>(std::abs(ticks));
        neg = true;
    }
    uint64_t subseconds = t % static_cast<uint64_t>(TimeSpan::TICKS_PER_SECOND);
    uint64_t seconds = t / static_cast<uint64_t>(TimeSpan::TICKS_PER_SECOND);
    std::string r = std::format("{}{}.{:07}", neg ? "-" : "", seconds, subseconds);
    while (r.ends_with('0')) {
        r.pop_back();
    }
    if (r.ends_with('.')) {
        r.pop_back();
    }
    return r;
}
} // namespace VodArchiver
