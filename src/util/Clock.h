#pragma once

#include <chrono>
#include <string>

namespace Lms {

/**
 * Interface for time abstraction, allowing tests to control "now".
 */
class Clock {
public:
    virtual ~Clock() = default;

    /**
     * Get the current time as ISO 8601 UTC string.
     */
    virtual std::string now() const = 0;

    /**
     * Get the current time as time_point.
     */
    virtual std::chrono::system_clock::time_point nowTimePoint() const = 0;
};

/**
 * Real clock implementation using system time.
 */
class SystemClock : public Clock {
public:
    std::string now() const override;
    std::chrono::system_clock::time_point nowTimePoint() const override;
};

/**
 * Fixed clock for testing, always returns a predetermined time.
 */
class FixedClock : public Clock {
public:
    explicit FixedClock(const std::string& fixedTime);
    explicit FixedClock(const std::chrono::system_clock::time_point& fixedTime);

    std::string now() const override;
    std::chrono::system_clock::time_point nowTimePoint() const override;

private:
    std::chrono::system_clock::time_point m_fixedTime;
};

/**
 * Utility functions for time conversion.
 */
namespace TimeUtils {
    std::string toIsoString(const std::chrono::system_clock::time_point& timePoint);
    std::chrono::system_clock::time_point fromIsoString(const std::string& isoString);
}

} // namespace Lms
