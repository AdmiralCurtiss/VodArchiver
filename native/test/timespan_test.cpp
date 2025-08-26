#include <cstdint>

#include <string>

#include "gtest/gtest.h"

#include "vodarchiver/time_types.h"

TEST(TimeSpan, ParseFromSeconds) {
    using VodArchiver::TimeSpan;
    static constexpr int64_t t = TimeSpan::TICKS_PER_SECOND;
    EXPECT_EQ(TimeSpan{.Ticks = 0}, TimeSpan::ParseFromSeconds("0"));
    EXPECT_EQ(TimeSpan{.Ticks = t}, TimeSpan::ParseFromSeconds("1"));
    EXPECT_EQ(TimeSpan{.Ticks = -t}, TimeSpan::ParseFromSeconds("-1"));
    EXPECT_EQ(TimeSpan{.Ticks = 0}, TimeSpan::ParseFromSeconds("0.0"));
    EXPECT_EQ(TimeSpan{.Ticks = t}, TimeSpan::ParseFromSeconds("1.0"));
    EXPECT_EQ(TimeSpan{.Ticks = -t}, TimeSpan::ParseFromSeconds("-1.0"));
    EXPECT_EQ(TimeSpan{.Ticks = t + t / 2}, TimeSpan::ParseFromSeconds("1.5"));
    EXPECT_EQ(TimeSpan{.Ticks = t + t / 2 + 1}, TimeSpan::ParseFromSeconds("1.5000001"));
    EXPECT_EQ(TimeSpan{.Ticks = -(t + t / 2)}, TimeSpan::ParseFromSeconds("-1.5"));
    EXPECT_EQ(TimeSpan{.Ticks = t + t / 8}, TimeSpan::ParseFromSeconds("1.125"));
    EXPECT_EQ(TimeSpan{.Ticks = -(3 * t) + (t / 8)}, TimeSpan::ParseFromSeconds("-2.875"));

    // number of digits until tick limit
    EXPECT_EQ(TimeSpan{.Ticks = 1000000}, TimeSpan::ParseFromSeconds("0.1"));
    EXPECT_EQ(TimeSpan{.Ticks = 1200000}, TimeSpan::ParseFromSeconds("0.12"));
    EXPECT_EQ(TimeSpan{.Ticks = 1230000}, TimeSpan::ParseFromSeconds("0.123"));
    EXPECT_EQ(TimeSpan{.Ticks = 1234000}, TimeSpan::ParseFromSeconds("0.1234"));
    EXPECT_EQ(TimeSpan{.Ticks = 1234500}, TimeSpan::ParseFromSeconds("0.12345"));
    EXPECT_EQ(TimeSpan{.Ticks = 1234560}, TimeSpan::ParseFromSeconds("0.123456"));
    EXPECT_EQ(TimeSpan{.Ticks = 1234567}, TimeSpan::ParseFromSeconds("0.1234567"));
    EXPECT_EQ(TimeSpan{.Ticks = -1000000}, TimeSpan::ParseFromSeconds("-0.1"));
    EXPECT_EQ(TimeSpan{.Ticks = -1200000}, TimeSpan::ParseFromSeconds("-0.12"));
    EXPECT_EQ(TimeSpan{.Ticks = -1230000}, TimeSpan::ParseFromSeconds("-0.123"));
    EXPECT_EQ(TimeSpan{.Ticks = -1234000}, TimeSpan::ParseFromSeconds("-0.1234"));
    EXPECT_EQ(TimeSpan{.Ticks = -1234500}, TimeSpan::ParseFromSeconds("-0.12345"));
    EXPECT_EQ(TimeSpan{.Ticks = -1234560}, TimeSpan::ParseFromSeconds("-0.123456"));
    EXPECT_EQ(TimeSpan{.Ticks = -1234567}, TimeSpan::ParseFromSeconds("-0.1234567"));

    // should round correctly if there are too many digits to be repesented by ticks
    EXPECT_EQ(TimeSpan{.Ticks = t + t / 2 + 0}, TimeSpan::ParseFromSeconds("1.50000001"));
    EXPECT_EQ(TimeSpan{.Ticks = t + t / 2 + 0}, TimeSpan::ParseFromSeconds("1.50000002"));
    EXPECT_EQ(TimeSpan{.Ticks = t + t / 2 + 0}, TimeSpan::ParseFromSeconds("1.50000003"));
    EXPECT_EQ(TimeSpan{.Ticks = t + t / 2 + 0}, TimeSpan::ParseFromSeconds("1.50000004"));
    EXPECT_EQ(TimeSpan{.Ticks = t + t / 2 + 1}, TimeSpan::ParseFromSeconds("1.50000005"));
    EXPECT_EQ(TimeSpan{.Ticks = t + t / 2 + 1}, TimeSpan::ParseFromSeconds("1.50000006"));
    EXPECT_EQ(TimeSpan{.Ticks = t + t / 2 + 1}, TimeSpan::ParseFromSeconds("1.50000007"));
    EXPECT_EQ(TimeSpan{.Ticks = t + t / 2 + 1}, TimeSpan::ParseFromSeconds("1.50000008"));
    EXPECT_EQ(TimeSpan{.Ticks = t + t / 2 + 1}, TimeSpan::ParseFromSeconds("1.50000009"));
    EXPECT_EQ(TimeSpan{.Ticks = -(t + t / 2 + 0)}, TimeSpan::ParseFromSeconds("-1.50000001"));
    EXPECT_EQ(TimeSpan{.Ticks = -(t + t / 2 + 0)}, TimeSpan::ParseFromSeconds("-1.50000002"));
    EXPECT_EQ(TimeSpan{.Ticks = -(t + t / 2 + 0)}, TimeSpan::ParseFromSeconds("-1.50000003"));
    EXPECT_EQ(TimeSpan{.Ticks = -(t + t / 2 + 0)}, TimeSpan::ParseFromSeconds("-1.50000004"));
    EXPECT_EQ(TimeSpan{.Ticks = -(t + t / 2 + 1)}, TimeSpan::ParseFromSeconds("-1.50000005"));
    EXPECT_EQ(TimeSpan{.Ticks = -(t + t / 2 + 1)}, TimeSpan::ParseFromSeconds("-1.50000006"));
    EXPECT_EQ(TimeSpan{.Ticks = -(t + t / 2 + 1)}, TimeSpan::ParseFromSeconds("-1.50000007"));
    EXPECT_EQ(TimeSpan{.Ticks = -(t + t / 2 + 1)}, TimeSpan::ParseFromSeconds("-1.50000008"));
    EXPECT_EQ(TimeSpan{.Ticks = -(t + t / 2 + 1)}, TimeSpan::ParseFromSeconds("-1.50000009"));

    // min/max
    EXPECT_EQ(TimeSpan{.Ticks = std::numeric_limits<int64_t>::max() - 1},
              TimeSpan::ParseFromSeconds("922337203685.4775806"));
    EXPECT_EQ(TimeSpan{.Ticks = std::numeric_limits<int64_t>::max()},
              TimeSpan::ParseFromSeconds("922337203685.4775807"));
    EXPECT_EQ(TimeSpan{.Ticks = std::numeric_limits<int64_t>::max()},
              TimeSpan::ParseFromSeconds("922337203685.4775808"));
    EXPECT_EQ(TimeSpan{.Ticks = std::numeric_limits<int64_t>::max()},
              TimeSpan::ParseFromSeconds("922337203686"));
    EXPECT_EQ(TimeSpan{.Ticks = std::numeric_limits<int64_t>::min() + 2},
              TimeSpan::ParseFromSeconds("-922337203685.4775806"));
    EXPECT_EQ(TimeSpan{.Ticks = std::numeric_limits<int64_t>::min() + 1},
              TimeSpan::ParseFromSeconds("-922337203685.4775807"));
    EXPECT_EQ(TimeSpan{.Ticks = std::numeric_limits<int64_t>::min()},
              TimeSpan::ParseFromSeconds("-922337203685.4775808"));
    EXPECT_EQ(TimeSpan{.Ticks = std::numeric_limits<int64_t>::min()},
              TimeSpan::ParseFromSeconds("-922337203685.4775809"));
    EXPECT_EQ(TimeSpan{.Ticks = std::numeric_limits<int64_t>::min()},
              TimeSpan::ParseFromSeconds("-922337203686"));

    // invalid stuff should fail to parse
    EXPECT_EQ(std::nullopt, TimeSpan::ParseFromSeconds("nonsense"));
    EXPECT_EQ(std::nullopt, TimeSpan::ParseFromSeconds("a1.1"));
    EXPECT_EQ(std::nullopt, TimeSpan::ParseFromSeconds("-1.1x"));
    EXPECT_EQ(std::nullopt, TimeSpan::ParseFromSeconds("1.5000000000z"));
}

TEST(TimeSpan, TimeSpanToTotalSecondsString) {
    using VodArchiver::TimeSpan;
    static constexpr int64_t t = TimeSpan::TICKS_PER_SECOND;
    EXPECT_EQ("0", TimeSpanToTotalSecondsString(TimeSpan{.Ticks = 0}));
    EXPECT_EQ("1", TimeSpanToTotalSecondsString(TimeSpan{.Ticks = t}));
    EXPECT_EQ("-1", TimeSpanToTotalSecondsString(TimeSpan{.Ticks = -t}));
    EXPECT_EQ("1.5", TimeSpanToTotalSecondsString(TimeSpan{.Ticks = t + t / 2}));
    EXPECT_EQ("1.5000001", TimeSpanToTotalSecondsString(TimeSpan{.Ticks = t + t / 2 + 1}));
    EXPECT_EQ("-1.5", TimeSpanToTotalSecondsString(TimeSpan{.Ticks = -(t + t / 2)}));
    EXPECT_EQ("1.125", TimeSpanToTotalSecondsString(TimeSpan{.Ticks = t + t / 8}));
    EXPECT_EQ("-2.875", TimeSpanToTotalSecondsString(TimeSpan{.Ticks = -(3 * t) + (t / 8)}));

    EXPECT_EQ("0.1", TimeSpanToTotalSecondsString(TimeSpan{.Ticks = 1000000}));
    EXPECT_EQ("0.12", TimeSpanToTotalSecondsString(TimeSpan{.Ticks = 1200000}));
    EXPECT_EQ("0.123", TimeSpanToTotalSecondsString(TimeSpan{.Ticks = 1230000}));
    EXPECT_EQ("0.1234", TimeSpanToTotalSecondsString(TimeSpan{.Ticks = 1234000}));
    EXPECT_EQ("0.12345", TimeSpanToTotalSecondsString(TimeSpan{.Ticks = 1234500}));
    EXPECT_EQ("0.123456", TimeSpanToTotalSecondsString(TimeSpan{.Ticks = 1234560}));
    EXPECT_EQ("0.1234567", TimeSpanToTotalSecondsString(TimeSpan{.Ticks = 1234567}));
    EXPECT_EQ("-0.1", TimeSpanToTotalSecondsString(TimeSpan{.Ticks = -1000000}));
    EXPECT_EQ("-0.12", TimeSpanToTotalSecondsString(TimeSpan{.Ticks = -1200000}));
    EXPECT_EQ("-0.123", TimeSpanToTotalSecondsString(TimeSpan{.Ticks = -1230000}));
    EXPECT_EQ("-0.1234", TimeSpanToTotalSecondsString(TimeSpan{.Ticks = -1234000}));
    EXPECT_EQ("-0.12345", TimeSpanToTotalSecondsString(TimeSpan{.Ticks = -1234500}));
    EXPECT_EQ("-0.123456", TimeSpanToTotalSecondsString(TimeSpan{.Ticks = -1234560}));
    EXPECT_EQ("-0.1234567", TimeSpanToTotalSecondsString(TimeSpan{.Ticks = -1234567}));

    EXPECT_EQ(
        "922337203685.4775806",
        TimeSpanToTotalSecondsString(TimeSpan{.Ticks = std::numeric_limits<int64_t>::max() - 1}));
    EXPECT_EQ("922337203685.4775807",
              TimeSpanToTotalSecondsString(TimeSpan{.Ticks = std::numeric_limits<int64_t>::max()}));
    EXPECT_EQ(
        "-922337203685.4775806",
        TimeSpanToTotalSecondsString(TimeSpan{.Ticks = std::numeric_limits<int64_t>::min() + 2}));
    EXPECT_EQ(
        "-922337203685.4775807",
        TimeSpanToTotalSecondsString(TimeSpan{.Ticks = std::numeric_limits<int64_t>::min() + 1}));
    EXPECT_EQ("-922337203685.4775808",
              TimeSpanToTotalSecondsString(TimeSpan{.Ticks = std::numeric_limits<int64_t>::min()}));
}
