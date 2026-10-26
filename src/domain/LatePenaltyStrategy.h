#pragma once

#include "Entities.h"
#include "util/Clock.h"
#include <memory>

namespace Lms {

/**
 * Interface for late penalty calculation strategies.
 */
class ILatePenaltyStrategy {
public:
    virtual ~ILatePenaltyStrategy() = default;

    /**
     * Calculate the penalty for a late submission.
     * @param submission The submission being penalized
     * @param assignment The assignment with deadline and max mark
     * @param clock Clock for getting current time
     * @return Penalty percentage (0.0 to 100.0)
     */
    virtual double calculatePenalty(const Submission& submission,
                                     const Assignment& assignment,
                                     const Clock& clock) const = 0;
};

/**
 * Standard late penalty: 10% of max mark per started 24-hour period for days 1-3,
 * 100% (zero mark) from day 4 onwards.
 */
class StandardLatePenalty : public ILatePenaltyStrategy {
public:
    double calculatePenalty(const Submission& submission,
                            const Assignment& assignment,
                            const Clock& clock) const override;
};

/**
 * No late penalty - submissions are never penalized.
 */
class NoLatePenalty : public ILatePenaltyStrategy {
public:
    double calculatePenalty(const Submission& submission,
                            const Assignment& assignment,
                            const Clock& clock) const override;
};

} // namespace Lms
