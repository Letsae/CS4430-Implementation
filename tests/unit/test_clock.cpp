#include <gtest/gtest.h>
#include "util/Clock.h"
#include "domain/LmsException.h"
#include <thread>

using namespace Lms;

TEST(Clock, SystemClockReturnsValidIsoString) {
    SystemClock clock;
    std::string time = clock.now();
    EXPECT_FALSE(time.empty());
    EXPECT_EQ(time.length(), 20); // YYYY-MM-DDTHH:MM:SSZ
    EXPECT_EQ(time[19], 'Z');
}

TEST(FixedClock, ReturnsFixedTime) {
    std::string fixedTime = "2024-01-15T10:30:00Z";
    FixedClock clock(fixedTime);
    EXPECT_EQ(clock.now(), fixedTime);
}

TEST(FixedClock, MultipleCallsReturnSameTime) {
    std::string fixedTime = "2024-01-15T10:30:00Z";
    FixedClock clock(fixedTime);
    EXPECT_EQ(clock.now(), clock.now());
}

TEST(TimeUtils, ToIsoString) {
    auto timePoint = std::chrono::system_clock::from_time_t(0); // 1970-01-01 00:00:00 UTC
    std::string iso = TimeUtils::toIsoString(timePoint);
    EXPECT_EQ(iso, "1970-01-01T00:00:00Z");
}

TEST(TimeUtils, FromIsoStringValid) {
    std::string iso = "2024-01-15T10:30:00Z";
    auto timePoint = TimeUtils::fromIsoString(iso);
    std::string converted = TimeUtils::toIsoString(timePoint);
    EXPECT_EQ(converted, iso);
}

TEST(TimeUtils, FromIsoStringInvalid) {
    std::string invalid = "not-a-date";
    EXPECT_THROW(TimeUtils::fromIsoString(invalid), ValidationException);
}

TEST(TimeUtils, FromIsoStringInvalidFormat) {
    std::string invalid = "2024-01-15"; // Missing time
    EXPECT_THROW(TimeUtils::fromIsoString(invalid), ValidationException);
}
