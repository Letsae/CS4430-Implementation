#include "SubmissionStateMachine.h"

namespace Lms {

void SubmissionStateMachine::transition(Submission& submission, SubmissionState newState) {
    SubmissionState currentState = submission.getState();

    if (!isValidTransition(currentState, newState)) {
        throw InvalidTransitionException(
            "Invalid state transition from " + stateToString(currentState) +
            " to " + stateToString(newState)
        );
    }

    submission.setState(newState);
}

bool SubmissionStateMachine::isValidTransition(SubmissionState from, SubmissionState to) {
    if (from == to) {
        return false; // No self-transitions
    }

    switch (from) {
        case SubmissionState::NotSubmitted:
            return to == SubmissionState::Submitted || to == SubmissionState::Late;

        case SubmissionState::Submitted:
            return to == SubmissionState::Marked;

        case SubmissionState::Late:
            return to == SubmissionState::Marked;

        case SubmissionState::Marked:
            return to == SubmissionState::Moderated || to == SubmissionState::Released;

        case SubmissionState::Moderated:
            return to == SubmissionState::Released;

        case SubmissionState::Released:
            return false; // Terminal state

        default:
            return false;
    }
}

std::string SubmissionStateMachine::stateToString(SubmissionState state) {
    switch (state) {
        case SubmissionState::NotSubmitted:
            return "NotSubmitted";
        case SubmissionState::Submitted:
            return "Submitted";
        case SubmissionState::Late:
            return "Late";
        case SubmissionState::Marked:
            return "Marked";
        case SubmissionState::Moderated:
            return "Moderated";
        case SubmissionState::Released:
            return "Released";
        default:
            return "Unknown";
    }
}

} // namespace Lms
