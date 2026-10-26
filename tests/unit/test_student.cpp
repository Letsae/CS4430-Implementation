#include <gtest/gtest.h>
#include "application/StudentService.h"
#include "fakes/FakeRepositories.h"
#include "util/Clock.h"

using namespace Lms;
using namespace Lms::Test;

class StudentServiceUnitTest : public ::testing::Test {
protected:
    void SetUp() override {
        clock = std::make_unique<FixedClock>("2024-01-15T10:00:00Z");

        deptRepo.departments.push_back(Department(1, "Computer Science"));
        deptRepo.departments.push_back(Department(2, "Mathematics"));

        courseRepo.courses.push_back(Course(1, "CS101", "Intro to Programming", 1));
        courseRepo.courses.push_back(Course(2, "MATH101", "Calculus I", 2));
        courseRepo.enroll(1, 1); // student 1 -> CS101
        courseRepo.enroll(2, 2); // student 2 -> MATH101
        courseRepo.lecturerNames[1] = {"alice", "bob"};
        courseRepo.lecturerNames[2] = {"carol"};

        assignmentRepo.assignments.push_back(
            Assignment(10, 1, "Essay", "2024-01-20T10:00:00Z", 100.0, 9));
        assignmentRepo.assignments.push_back(
            Assignment(11, 2, "Problem set", "2024-01-22T10:00:00Z", 50.0, 9));

        service = std::make_unique<StudentService>(
            courseRepo, assignmentRepo, submissionRepo, versionRepo, extensionRepo,
            deptRepo, userRepo, *clock);
    }

    FakeCourseRepository courseRepo;
    FakeAssignmentRepository assignmentRepo;
    FakeSubmissionRepository submissionRepo;
    FakeSubmissionVersionRepository versionRepo;
    FakeExtensionRepository extensionRepo;
    FakeDepartmentRepository deptRepo;
    FakeUserRepository userRepo;
    std::unique_ptr<FixedClock> clock;
    std::unique_ptr<StudentService> service;
};

TEST_F(StudentServiceUnitTest, ListCoursesReturnsOnlyOwnEnrolments) {
    auto courses = service->listCourses(1);

    ASSERT_EQ(courses.size(), 1u);
    EXPECT_EQ(courses[0].code, "CS101");
    EXPECT_EQ(courses[0].title, "Intro to Programming");
    EXPECT_EQ(courses[0].department, "Computer Science");
    ASSERT_EQ(courses[0].lecturers.size(), 2u);
    EXPECT_EQ(courses[0].lecturers[0], "alice");
    EXPECT_EQ(courses[0].lecturers[1], "bob");
}

TEST_F(StudentServiceUnitTest, ListCoursesDoesNotIncludeOtherStudentsCourses) {
    auto student1 = service->listCourses(1);
    auto student2 = service->listCourses(2);

    ASSERT_EQ(student1.size(), 1u);
    ASSERT_EQ(student2.size(), 1u);
    EXPECT_EQ(student1[0].code, "CS101");
    EXPECT_EQ(student2[0].code, "MATH101");
}

TEST_F(StudentServiceUnitTest, ListCoursesEmptyWhenNotEnrolled) {
    auto courses = service->listCourses(99);
    EXPECT_TRUE(courses.empty());
}

TEST_F(StudentServiceUnitTest, EffectiveDeadlineWithoutExtensionIsAssignmentDeadline) {
    auto courses = service->listCourses(1);
    ASSERT_EQ(courses[0].assignments.size(), 1u);
    EXPECT_FALSE(courses[0].assignments[0].hasExtension);
    EXPECT_EQ(courses[0].assignments[0].effectiveDeadline, "2024-01-20T10:00:00Z");
    EXPECT_EQ(courses[0].assignments[0].state, "NotSubmitted");
    EXPECT_DOUBLE_EQ(courses[0].assignments[0].maxMark, 100.0);
}

TEST_F(StudentServiceUnitTest, EffectiveDeadlineUsesStudentExtension) {
    extensionRepo.extensions.push_back(
        Extension(1, 10, 1, "2024-01-25T10:00:00Z", "Medical", 9));

    auto courses = service->listCourses(1);
    ASSERT_EQ(courses[0].assignments.size(), 1u);
    EXPECT_TRUE(courses[0].assignments[0].hasExtension);
    EXPECT_EQ(courses[0].assignments[0].effectiveDeadline, "2024-01-25T10:00:00Z");
}

TEST_F(StudentServiceUnitTest, ExtensionForAnotherStudentIsIgnored) {
    extensionRepo.extensions.push_back(
        Extension(1, 10, 2, "2024-01-25T10:00:00Z", "Medical", 9));

    auto courses = service->listCourses(1);
    ASSERT_EQ(courses[0].assignments.size(), 1u);
    EXPECT_FALSE(courses[0].assignments[0].hasExtension);
    EXPECT_EQ(courses[0].assignments[0].effectiveDeadline, "2024-01-20T10:00:00Z");
}

TEST_F(StudentServiceUnitTest, AssignmentStateReflectsOwnSubmissionOnly) {
    Submission other(1, 10, 2, SubmissionState::Submitted);
    other.setFirstSubmittedAt("2024-01-14T10:00:00Z");
    submissionRepo.submissions.push_back(other);

    Submission own(2, 10, 1, SubmissionState::Late);
    own.setFirstSubmittedAt("2024-01-21T10:00:00Z");
    submissionRepo.submissions.push_back(own);

    auto courses = service->listCourses(1);
    ASSERT_EQ(courses[0].assignments.size(), 1u);
    EXPECT_EQ(courses[0].assignments[0].state, "Late");
}
