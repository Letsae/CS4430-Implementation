#pragma once

#include "Entities.h"
#include "util/Clock.h"
#include <memory>
#include <chrono>

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
 * Standard late penalty: configured percentage for each started day, capped by
 * the configured number of penalty days. Later work remains accepted at zero penalty.
 */
class StandardLatePenalty : public ILatePenaltyStrategy {
public:
    explicit StandardLatePenalty(double percentPerDay = 10.0, int maximumPenaltyDays = 3,
                                 int secondsPerDay = 86400);
    double calculatePenalty(const Submission& submission,
                            const Assignment& assignment,
                            const Clock& clock) const override;

private:
    double m_percentPerDay;
    int m_maximumPenaltyDays;
    int m_secondsPerDay;
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
