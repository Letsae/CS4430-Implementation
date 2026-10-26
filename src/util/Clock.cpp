#include "Clock.h"
#include "domain/LmsException.h"
#include <iomanip>
#include <sstream>
#include <ctime>

namespace Lms {

std::string SystemClock::now() const {
    return TimeUtils::toIsoString(nowTimePoint());
}

std::chrono::system_clock::time_point SystemClock::nowTimePoint() const {
    return std::chrono::system_clock::now();
}

FixedClock::FixedClock(const std::string& fixedTime)
    : m_fixedTime(TimeUtils::fromIsoString(fixedTime)) {}

FixedClock::FixedClock(const std::chrono::system_clock::time_point& fixedTime)
    : m_fixedTime(fixedTime) {}

std::string FixedClock::now() const {
    return TimeUtils::toIsoString(m_fixedTime);
}

std::chrono::system_clock::time_point FixedClock::nowTimePoint() const {
    return m_fixedTime;
}

namespace TimeUtils {

std::string toIsoString(const std::chrono::system_clock::time_point& timePoint) {
    auto time = std::chrono::system_clock::to_time_t(timePoint);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

std::chrono::system_clock::time_point fromIsoString(const std::string& isoString) {
    // Parse YYYY-MM-DDTHH:MM:SSZ without relying on get_time consuming a trailing 'Z'
    // (MinGW/libstdc++ leaves 'Z' unparsed or treats %Z as a timezone name).
    if (isoString.size() != 20 || isoString[10] != 'T' || isoString.back() != 'Z') {
        throw ValidationException("Invalid ISO 8601 timestamp format: " + isoString);
    }

    std::tm tm = {};
    std::istringstream ss(isoString.substr(0, 19));
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (ss.fail()) {
        throw ValidationException("Invalid ISO 8601 timestamp format: " + isoString);
    }
    
    // Parse as UTC time
    time_t time;
#ifdef _WIN32
    // Windows: use _mkgmtime which interprets tm as UTC
    time = _mkgmtime(&tm);
    if (time == -1) {
        throw ValidationException("Invalid date values in timestamp: " + isoString);
    }
#else
    // Unix: use timegm which interprets tm as UTC
    #ifdef __APPLE__
        // macOS doesn't have timegm, use mktime with TZ set
        char* tz = getenv("TZ");
        setenv("TZ", "UTC", 1);
        tzset();
        time = mktime(&tm);
        if (tz) {
            setenv("TZ", tz, 1);
        } else {
            unsetenv("TZ");
        }
        tzset();
    #else
        time = timegm(&tm);
        if (time == -1) {
            throw ValidationException("Invalid date values in timestamp: " + isoString);
        }
    #endif
#endif
    return std::chrono::system_clock::from_time_t(time);
}

} // namespace TimeUtils

} // namespace Lms
