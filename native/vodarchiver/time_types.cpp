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
static auto DateTimeToTaiTimePoint(const DateTime& dt) {
    static constexpr int64_t EPOCH_DIFFERENCE_IN_TICKS = 617569056000000000;
    std::chrono::duration<int64_t, std::ratio<1, 10000000>> chrono_ticks(
        dt.GetTicks() - EPOCH_DIFFERENCE_IN_TICKS);
    std::chrono::time_point<std::chrono::tai_clock> epoch;
    auto target = (epoch + chrono_ticks);
    return target;
}

static DateTime DateTimeFromTaiTimePoint(
    const std::chrono::time_point<std::chrono::tai_clock,
                                  std::chrono::duration<int64_t, std::ratio<1, 10000000>>>& tp) {
    int64_t ticks = tp.time_since_epoch().count();
    static constexpr int64_t EPOCH_DIFFERENCE_IN_TICKS = 617569056000000000;
    return DateTime::FromTicksAndKind(static_cast<uint64_t>(ticks + EPOCH_DIFFERENCE_IN_TICKS),
                                      DateTime::KIND_UTC);
}

DateTime DateTime::UtcNow() {
    auto now = std::chrono::tai_clock::now();
    auto casted =
        std::chrono::time_point_cast<std::chrono::duration<int64_t, std::ratio<1, 10000000>>>(now);
    return DateTimeFromTaiTimePoint(casted);
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

DateTime DateTime::AddTicks(int64_t ticks) const {
    return DateTime::FromTicksAndKind(this->GetTicks() + ticks, this->GetKind());
}

DateTime DateTime::AddSeconds(int64_t seconds) const {
    int64_t ticks = static_cast<int64_t>(seconds) * static_cast<int64_t>(TICKS_PER_SECOND);
    return AddTicks(ticks);
}

DateTime DateTime::AddMinutes(int minutes) const {
    int64_t ticks = static_cast<int64_t>(minutes) * static_cast<int64_t>(60u * TICKS_PER_SECOND);
    return AddTicks(ticks);
}

DateTime DateTime::AddHours(int hours) const {
    int64_t ticks = static_cast<int64_t>(hours)
                    * static_cast<int64_t>(60u * 60u * static_cast<int64_t>(TICKS_PER_SECOND));
    return AddTicks(ticks);
}

static bool UnsignedMultiplyOverflows(uint64_t a, uint64_t b, uint64_t* low) {
#ifdef _MSC_VER
    uint64_t high;
    *low = _umul128(a, b, &high);
    bool overflow = (high != 0);
#else
    bool overflow = __builtin_mul_overflow(a, b, low);
#endif
    return overflow;
}

static TimeSpan
    FromUnsignedSecondsAndSubsecondTicks(uint64_t seconds, uint32_t subseconds, bool isNegative) {
    uint64_t ticks;
    bool overflow = UnsignedMultiplyOverflows(
        seconds, static_cast<uint64_t>(TimeSpan::TICKS_PER_SECOND), &ticks);
    if (overflow) {
        // overflow in the multiply, return max value
        if (isNegative) {
            return TimeSpan{.Ticks = std::numeric_limits<int64_t>::min()};
        } else {
            return TimeSpan{.Ticks = std::numeric_limits<int64_t>::max()};
        }
    }
    if (isNegative) {
        static constexpr uint64_t max = static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
        if (ticks > (max - subseconds)) {
            // this includes the case where it's exactly the minimum int64
            return TimeSpan{.Ticks = std::numeric_limits<int64_t>::min()};
        }
        return TimeSpan{.Ticks = -static_cast<int64_t>(ticks + subseconds)};
    } else {
        static constexpr uint64_t max = static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
        if (ticks > (max - subseconds)) {
            return TimeSpan{.Ticks = std::numeric_limits<int64_t>::max()};
        }
        return TimeSpan{.Ticks = static_cast<int64_t>(ticks + subseconds)};
    }
}

TimeSpan TimeSpan::FromIntegerSeconds(int64_t seconds) {
    if (seconds < 0) {
        return FromUnsignedSecondsAndSubsecondTicks(static_cast<uint64_t>(~seconds) + 1u, 0, true);
    } else {
        return FromUnsignedSecondsAndSubsecondTicks(static_cast<uint64_t>(seconds), 0, false);
    }
}

TimeSpan TimeSpan::FromDoubleSeconds(double seconds) {
    int64_t integer_seconds = static_cast<int64_t>(seconds);
    int64_t sub_integer_ticks = static_cast<int64_t>(
        std::llround(std::fmod(std::abs(seconds) * static_cast<double>(TICKS_PER_SECOND),
                               static_cast<double>(TICKS_PER_SECOND))));
    return TimeSpan{.Ticks = integer_seconds * static_cast<int64_t>(TICKS_PER_SECOND)
                             + sub_integer_ticks};
}

std::optional<TimeSpan> TimeSpan::ParseFromSeconds(std::string_view value) {
    if (auto dotpos = value.find('.'); dotpos != std::string_view::npos) {
        // try to parse the seconds and subsections separately so we don't lose information from the
        // double conversion
        std::string_view left = value.substr(0, dotpos);
        std::string_view right = value.substr(dotpos + 1);
        std::optional<uint64_t> sec;
        bool isNegative;
        if (left.empty()) {
            sec = 0;
            isNegative = false;
        } else if (left[0] == '-') {
            if (left.size() == 1) {
                sec = 0;
            } else {
                sec = HyoutaUtils::NumberUtils::ParseUInt64(left.substr(1));
            }
            isNegative = true;
        } else {
            sec = HyoutaUtils::NumberUtils::ParseUInt64(left);
            isNegative = false;
        }
        if (sec) {
            if (right.size() == 0) {
                return FromUnsignedSecondsAndSubsecondTicks(*sec, 0, isNegative);
            } else {
                if (right.size() <= 7) {
                    if (auto subsec = HyoutaUtils::NumberUtils::ParseUInt32(right)) {
                        // pad to 7 digits
                        for (size_t i = 0; i < (7 - right.size()); ++i) {
                            *subsec *= 10;
                        }
                        return FromUnsignedSecondsAndSubsecondTicks(*sec, *subsec, isNegative);
                    }
                } else {
                    // 8 or more digits. we truncate this to 8 digits and then round the last
                    // digit.
                    std::string_view truncated = right.substr(0, 8);

                    // the truncated-away digits must still be digits though
                    bool valid = true;
                    for (char c : right.substr(8)) {
                        if (!(c >= '0' && c <= '9')) {
                            valid = false;
                            break;
                        }
                    }

                    if (valid) {
                        if (auto subsec = HyoutaUtils::NumberUtils::ParseUInt32(truncated)) {
                            uint32_t rounded = *subsec / 10u;
                            uint32_t rest = *subsec % 10u;
                            if (rest >= 5) {
                                ++rounded;
                            }
                            return FromUnsignedSecondsAndSubsecondTicks(*sec, rounded, isNegative);
                        }
                    }
                }
            }
        }
    }
    if (auto l = HyoutaUtils::NumberUtils::ParseInt64(value)) {
        return TimeSpan::FromIntegerSeconds(*l);
    }
    if (auto l = HyoutaUtils::NumberUtils::ParseDouble(value)) {
        return TimeSpan::FromDoubleSeconds(*l);
    }
    return std::nullopt;
}

std::string DateTimeToStringForFilesystem(DateTime dt) {
    auto target = DateTimeToTaiTimePoint(dt);
    uint64_t seconds = (dt.GetTicks() / DateTime::TICKS_PER_SECOND) % 60;
    return std::format("{0:%Y}-{0:%m}-{0:%d}_{0:%H}-{0:%M}-{1:02}", target, seconds);
}

std::string_view DateTimeToStringForGui(DateTime dt, std::array<char, 24>& buffer) {
    auto target = DateTimeToTaiTimePoint(dt);
    uint64_t seconds = (dt.GetTicks() / DateTime::TICKS_PER_SECOND) % 60;
    auto result = std::format_to_n(buffer.data(),
                                   buffer.size() - 1,
                                   "{0:%Y}-{0:%m}-{0:%d} {0:%H}:{0:%M}:{1:02}",
                                   target,
                                   seconds);
    *result.out = '\0';
    return std::string_view(buffer.data(), result.out);
}

std::string DateToString(DateTime dt) {
    auto target = DateTimeToTaiTimePoint(dt);
    return std::format("{0:%Y}-{0:%m}-{0:%d}", target);
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
