#include "LatePenaltyStrategy.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include "LmsException.h"

namespace Lms {

StandardLatePenalty::StandardLatePenalty(double percentPerDay, int maximumPenaltyDays,
                                         int secondsPerDay)
    : m_percentPerDay(percentPerDay), m_maximumPenaltyDays(maximumPenaltyDays),
      m_secondsPerDay(secondsPerDay) {
    if (!std::isfinite(percentPerDay) || percentPerDay < 0.0 || percentPerDay > 100.0 ||
        maximumPenaltyDays < 0 || maximumPenaltyDays > 3 || secondsPerDay <= 0)
        throw ValidationException("Late penalty settings are outside supported limits");
}

double StandardLatePenalty::calculatePenalty(const Submission& submission,
                                             const Assignment& assignment,
                                             const Clock& clock) const {
    (void)clock; // Unused in standard penalty calculation
    std::string deadline = assignment.getDeadline();
    auto deadlineTime = TimeUtils::fromIsoString(deadline);
    auto submittedTime = TimeUtils::fromIsoString(submission.getFirstSubmittedAt());

    auto duration = std::chrono::duration_cast<std::chrono::seconds>(submittedTime - deadlineTime);
    double secondsLate = static_cast<double>(duration.count());

    if (secondsLate <= 0) {
        return 0.0;
    }

    // Convert to configured day periods and round up to started days.
    double daysLate = secondsLate / static_cast<double>(m_secondsPerDay);

    // Ceiling to get started days
    int daysLateCeiled = static_cast<int>(std::ceil(daysLate));

    if (daysLateCeiled >= 1 && daysLateCeiled <= m_maximumPenaltyDays) {
        return std::min(100.0, daysLateCeiled * m_percentPerDay);
    }

    // DR2 caps the penalty at the first three started 24-hour periods.
    // Late work from day four onward is still accepted without a penalty.
    return 0.0;
}

double NoLatePenalty::calculatePenalty(const Submission& submission,
                                        const Assignment& assignment,
                                        const Clock& clock) const {
    (void)submission;
    (void)assignment;
    (void)clock;
    return 0.0;
}

} // namespace Lms
