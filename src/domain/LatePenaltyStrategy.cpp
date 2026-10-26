#include "LatePenaltyStrategy.h"
#include <cmath>
#include <chrono>

namespace Lms {

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

    // Convert to days (86400 seconds per day)
    double daysLate = secondsLate / 86400.0;

    // Ceiling to get started days
    int daysLateCeiled = static_cast<int>(std::ceil(daysLate));

    if (daysLateCeiled >= 1 && daysLateCeiled <= 3) {
        return daysLateCeiled * 10.0;
    }

    if (daysLateCeiled >= 4) {
        return 100.0;
    }

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
