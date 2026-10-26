#include <gtest/gtest.h>
#include "domain/SubmissionStateMachine.h"
#include "domain/Entities.h"

using namespace Lms;

class SubmissionStateMachineTest : public ::testing::Test {
protected:
    Submission createSubmission(SubmissionState state) {
        return Submission(1, 1, 1, state);
    }
};

TEST_F(SubmissionStateMachineTest, ValidTransition_NotSubmittedToSubmitted) {
    Submission submission = createSubmission(SubmissionState::NotSubmitted);
    EXPECT_NO_THROW(SubmissionStateMachine::transition(submission, SubmissionState::Submitted));
    EXPECT_EQ(submission.getState(), SubmissionState::Submitted);
}

TEST_F(SubmissionStateMachineTest, ValidTransition_NotSubmittedToLate) {
    Submission submission = createSubmission(SubmissionState::NotSubmitted);
    EXPECT_NO_THROW(SubmissionStateMachine::transition(submission, SubmissionState::Late));
    EXPECT_EQ(submission.getState(), SubmissionState::Late);
}

TEST_F(SubmissionStateMachineTest, ValidTransition_SubmittedToMarked) {
    Submission submission = createSubmission(SubmissionState::Submitted);
    EXPECT_NO_THROW(SubmissionStateMachine::transition(submission, SubmissionState::Marked));
    EXPECT_EQ(submission.getState(), SubmissionState::Marked);
}

TEST_F(SubmissionStateMachineTest, ValidTransition_LateToMarked) {
    Submission submission = createSubmission(SubmissionState::Late);
    EXPECT_NO_THROW(SubmissionStateMachine::transition(submission, SubmissionState::Marked));
    EXPECT_EQ(submission.getState(), SubmissionState::Marked);
}

TEST_F(SubmissionStateMachineTest, ValidTransition_MarkedToModerated) {
    Submission submission = createSubmission(SubmissionState::Marked);
    EXPECT_NO_THROW(SubmissionStateMachine::transition(submission, SubmissionState::Moderated));
    EXPECT_EQ(submission.getState(), SubmissionState::Moderated);
}

TEST_F(SubmissionStateMachineTest, ValidTransition_MarkedToReleased) {
    Submission submission = createSubmission(SubmissionState::Marked);
    EXPECT_NO_THROW(SubmissionStateMachine::transition(submission, SubmissionState::Released));
    EXPECT_EQ(submission.getState(), SubmissionState::Released);
}

TEST_F(SubmissionStateMachineTest, ValidTransition_ModeratedToReleased) {
    Submission submission = createSubmission(SubmissionState::Moderated);
    EXPECT_NO_THROW(SubmissionStateMachine::transition(submission, SubmissionState::Released));
    EXPECT_EQ(submission.getState(), SubmissionState::Released);
}

TEST_F(SubmissionStateMachineTest, InvalidTransition_SelfTransition) {
    Submission submission = createSubmission(SubmissionState::Submitted);
    EXPECT_THROW(
        SubmissionStateMachine::transition(submission, SubmissionState::Submitted),
        InvalidTransitionException
    );
}

TEST_F(SubmissionStateMachineTest, InvalidTransition_SubmittedToLate) {
    Submission submission = createSubmission(SubmissionState::Submitted);
    EXPECT_THROW(
        SubmissionStateMachine::transition(submission, SubmissionState::Late),
        InvalidTransitionException
    );
}

TEST_F(SubmissionStateMachineTest, InvalidTransition_MarkedToSubmitted) {
    Submission submission = createSubmission(SubmissionState::Marked);
    EXPECT_THROW(
        SubmissionStateMachine::transition(submission, SubmissionState::Submitted),
        InvalidTransitionException
    );
}

TEST_F(SubmissionStateMachineTest, InvalidTransition_ReleasedToMarked) {
    Submission submission = createSubmission(SubmissionState::Released);
    EXPECT_THROW(
        SubmissionStateMachine::transition(submission, SubmissionState::Marked),
        InvalidTransitionException
    );
}

TEST_F(SubmissionStateMachineTest, InvalidTransition_NotSubmittedToMarked) {
    Submission submission = createSubmission(SubmissionState::NotSubmitted);
    EXPECT_THROW(
        SubmissionStateMachine::transition(submission, SubmissionState::Marked),
        InvalidTransitionException
    );
}

TEST_F(SubmissionStateMachineTest, InvalidTransition_SubmittedToReleased) {
    Submission submission = createSubmission(SubmissionState::Submitted);
    EXPECT_THROW(
        SubmissionStateMachine::transition(submission, SubmissionState::Released),
        InvalidTransitionException
    );
}

TEST_F(SubmissionStateMachineTest, InvalidTransition_LateToReleased) {
    Submission submission = createSubmission(SubmissionState::Late);
    EXPECT_THROW(
        SubmissionStateMachine::transition(submission, SubmissionState::Released),
        InvalidTransitionException
    );
}

TEST_F(SubmissionStateMachineTest, InvalidTransition_ModeratedToMarked) {
    Submission submission = createSubmission(SubmissionState::Moderated);
    EXPECT_THROW(
        SubmissionStateMachine::transition(submission, SubmissionState::Marked),
        InvalidTransitionException
    );
}

TEST(IsValidTransition, ReturnsTrueForValidTransitions) {
    EXPECT_TRUE(SubmissionStateMachine::isValidTransition(
        SubmissionState::NotSubmitted, SubmissionState::Submitted
    ));
    EXPECT_TRUE(SubmissionStateMachine::isValidTransition(
        SubmissionState::NotSubmitted, SubmissionState::Late
    ));
    EXPECT_TRUE(SubmissionStateMachine::isValidTransition(
        SubmissionState::Submitted, SubmissionState::Marked
    ));
    EXPECT_TRUE(SubmissionStateMachine::isValidTransition(
        SubmissionState::Late, SubmissionState::Marked
    ));
    EXPECT_TRUE(SubmissionStateMachine::isValidTransition(
        SubmissionState::Marked, SubmissionState::Moderated
    ));
    EXPECT_TRUE(SubmissionStateMachine::isValidTransition(
        SubmissionState::Marked, SubmissionState::Released
    ));
    EXPECT_TRUE(SubmissionStateMachine::isValidTransition(
        SubmissionState::Moderated, SubmissionState::Released
    ));
}

TEST(IsValidTransition, ReturnsFalseForInvalidTransitions) {
    EXPECT_FALSE(SubmissionStateMachine::isValidTransition(
        SubmissionState::Submitted, SubmissionState::Submitted
    ));
    EXPECT_FALSE(SubmissionStateMachine::isValidTransition(
        SubmissionState::Released, SubmissionState::Marked
    ));
    EXPECT_FALSE(SubmissionStateMachine::isValidTransition(
        SubmissionState::NotSubmitted, SubmissionState::Marked
    ));
}

TEST(StateToString, ReturnsCorrectStrings) {
    EXPECT_EQ(SubmissionStateMachine::stateToString(SubmissionState::NotSubmitted), "NotSubmitted");
    EXPECT_EQ(SubmissionStateMachine::stateToString(SubmissionState::Submitted), "Submitted");
    EXPECT_EQ(SubmissionStateMachine::stateToString(SubmissionState::Late), "Late");
    EXPECT_EQ(SubmissionStateMachine::stateToString(SubmissionState::Marked), "Marked");
    EXPECT_EQ(SubmissionStateMachine::stateToString(SubmissionState::Moderated), "Moderated");
    EXPECT_EQ(SubmissionStateMachine::stateToString(SubmissionState::Released), "Released");
}
