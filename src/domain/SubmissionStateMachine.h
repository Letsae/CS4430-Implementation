#pragma once

#include "Entities.h"
#include "LmsException.h"
#include <string>

namespace Lms {

/**
 * State machine for Submission lifecycle.
 * Valid transitions:
 * - NotSubmitted -> Submitted
 * - NotSubmitted -> Late
 * - Submitted -> Marked
 * - Late -> Marked
 * - Marked -> Moderated
 * - Moderated -> Released
 * - Marked -> Released (if HoD approves without moderation)
 */
class SubmissionStateMachine {
public:
    /**
     * Transition the submission state.
     * @throws InvalidTransitionException if the transition is invalid
     */
    static void transition(Submission& submission, SubmissionState newState);

    /**
     * Check if a transition is valid.
     */
    static bool isValidTransition(SubmissionState from, SubmissionState to);

    /**
     * Get a human-readable description of the state.
     */
    static std::string stateToString(SubmissionState state);
};

} // namespace Lms
