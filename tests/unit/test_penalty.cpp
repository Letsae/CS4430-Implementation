#include <gtest/gtest.h>
#include "domain/LatePenaltyStrategy.h"
#include "domain/Entities.h"
#include "util/Clock.h"
#include <cmath>

using namespace Lms;

class PenaltyStrategyTest : public ::testing::Test {
protected:
    Assignment createAssignment(double maxMark = 100.0) {
        return Assignment(1, 1, "Test Assignment", "2024-01-15T10:00:00Z", maxMark, 1);
    }

    Submission createSubmission(const std::string& submittedAt) {
        Submission submission(1, 1, 1, SubmissionState::Submitted);
        submission.setFirstSubmittedAt(submittedAt);
        return submission;
    }

    StandardLatePenalty m_strategy;
    FixedClock m_clock{"2024-01-15T10:00:00Z"};
};

TEST_F(PenaltyStrategyTest, AtDeadline_NoPenalty) {
    auto assignment = createAssignment(100.0);
    auto submission = createSubmission("2024-01-15T10:00:00Z"); // Exactly at deadline

    double penalty = m_strategy.calculatePenalty(submission, assignment, m_clock);
    EXPECT_DOUBLE_EQ(penalty, 0.0);
}

TEST_F(PenaltyStrategyTest, OneSecondLate_OneDayPenalty) {
    auto assignment = createAssignment(100.0);
    auto submission = createSubmission("2024-01-15T10:00:01Z"); // 1 second late

    double penalty = m_strategy.calculatePenalty(submission, assignment, m_clock);
    EXPECT_DOUBLE_EQ(penalty, 10.0); // 1 day = 10%
}

TEST_F(PenaltyStrategyTest, Exactly24HoursLate_OneDayPenalty) {
    auto assignment = createAssignment(100.0);
    auto submission = createSubmission("2024-01-16T10:00:00Z"); // Exactly 24 hours late

    double penalty = m_strategy.calculatePenalty(submission, assignment, m_clock);
    EXPECT_DOUBLE_EQ(penalty, 10.0);
}

TEST_F(PenaltyStrategyTest, 24HoursAndOneSecondLate_TwoDayPenalty) {
    auto assignment = createAssignment(100.0);
    auto submission = createSubmission("2024-01-16T10:00:01Z"); // 24h + 1s late

    double penalty = m_strategy.calculatePenalty(submission, assignment, m_clock);
    EXPECT_DOUBLE_EQ(penalty, 20.0); // 2 days = 20%
}

TEST_F(PenaltyStrategyTest, 48HoursLate_TwoDayPenalty) {
    auto assignment = createAssignment(100.0);
    auto submission = createSubmission("2024-01-17T10:00:00Z"); // Exactly 48 hours late

    double penalty = m_strategy.calculatePenalty(submission, assignment, m_clock);
    EXPECT_DOUBLE_EQ(penalty, 20.0);
}

TEST_F(PenaltyStrategyTest, 72HoursLate_ThreeDayPenalty) {
    auto assignment = createAssignment(100.0);
    auto submission = createSubmission("2024-01-18T10:00:00Z"); // Exactly 72 hours late

    double penalty = m_strategy.calculatePenalty(submission, assignment, m_clock);
    EXPECT_DOUBLE_EQ(penalty, 30.0); // 3 days = 30%
}

TEST_F(PenaltyStrategyTest, 72HoursAndOneSecondLate_ZeroMark) {
    auto assignment = createAssignment(100.0);
    auto submission = createSubmission("2024-01-18T10:00:01Z"); // 72h + 1s late

    double penalty = m_strategy.calculatePenalty(submission, assignment, m_clock);
    EXPECT_DOUBLE_EQ(penalty, 100.0); // Day 4+ = 100% penalty
}

TEST_F(PenaltyStrategyTest, FourDaysLate_ZeroMark) {
    auto assignment = createAssignment(100.0);
    auto submission = createSubmission("2024-01-19T10:00:00Z"); // 96 hours late

    double penalty = m_strategy.calculatePenalty(submission, assignment, m_clock);
    EXPECT_DOUBLE_EQ(penalty, 100.0);
}

TEST_F(PenaltyStrategyTest, EarlySubmission_NoPenalty) {
    auto assignment = createAssignment(100.0);
    auto submission = createSubmission("2024-01-15T09:59:59Z"); // 1 second early

    double penalty = m_strategy.calculatePenalty(submission, assignment, m_clock);
    EXPECT_DOUBLE_EQ(penalty, 0.0);
}

TEST_F(PenaltyStrategyTest, DifferentMaxMark_PenaltyPercentageSame) {
    auto assignment = createAssignment(50.0); // Max mark 50
    auto submission = createSubmission("2024-01-15T10:00:01Z"); // 1 second late

    double penalty = m_strategy.calculatePenalty(submission, assignment, m_clock);
    EXPECT_DOUBLE_EQ(penalty, 10.0); // Still 10% penalty
}

TEST(NoLatePenalty, AlwaysZero) {
    NoLatePenalty strategy;
    Assignment assignment(1, 1, "Test", "2024-01-15T10:00:00Z", 100.0, 1);
    Submission submission(1, 1, 1, SubmissionState::Late);
    submission.setFirstSubmittedAt("2024-01-20T10:00:00Z"); // 5 days late
    FixedClock clock{"2024-01-15T10:00:00Z"};

    double penalty = strategy.calculatePenalty(submission, assignment, clock);
    EXPECT_DOUBLE_EQ(penalty, 0.0);
}
