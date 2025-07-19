#include "time_types.h"

#include <chrono>
#include <format>

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

std::string DateTimeToStringForFilesystem(DateTime dt) {
    static constexpr int64_t EPOCH_DIFFERENCE_IN_TICKS = 617569056000000000;
    uint64_t seconds = (dt.Data / DateTime::TICKS_PER_SECOND) % 60;
    std::chrono::nanoseconds ns((dt.Data - EPOCH_DIFFERENCE_IN_TICKS) * 100);
    std::chrono::time_point<std::chrono::tai_clock> epoch;
    auto target = (epoch + ns);
    return std::format("{0:%Y}-{0:%m}-{0:%d}_{0:%H}-{0:%M}-{1:02}", target, seconds);
}

std::string DateTimeToStringForGui(DateTime dt) {
    static constexpr int64_t EPOCH_DIFFERENCE_IN_TICKS = 617569056000000000;
    uint64_t seconds = (dt.Data / DateTime::TICKS_PER_SECOND) % 60;
    std::chrono::nanoseconds ns((dt.Data - EPOCH_DIFFERENCE_IN_TICKS) * 100);
    std::chrono::time_point<std::chrono::tai_clock> epoch;
    auto target = (epoch + ns);
    return std::format("{0:%Y}-{0:%m}-{0:%d} {0:%H}:{0:%M}:{1:02}", target, seconds);
}

std::string DateToString(DateTime dt) {
    static constexpr int64_t EPOCH_DIFFERENCE_IN_TICKS = 617569056000000000;
    std::chrono::nanoseconds ns((dt.Data - EPOCH_DIFFERENCE_IN_TICKS) * 100);
    std::chrono::time_point<std::chrono::tai_clock> epoch;
    auto target = (epoch + ns);
    return std::format("{0:%Y}-{0:%m}-{0:%d}", target);
}
} // namespace VodArchiver
