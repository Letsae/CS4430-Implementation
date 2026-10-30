#include <gtest/gtest.h>
#include "application/StudentService.h"
#include "fakes/FakeRepositories.h"
#include "util/Clock.h"

using namespace Lms;
using namespace Lms::Test;

namespace {
constexpr int kCsCourseId = 1;
constexpr int kMathCourseId = 2;
constexpr int kEssayAssignmentId = 10;
constexpr int kProblemSetAssignmentId = 11;
constexpr int kLecturerUserId = 9;
constexpr char kTestHash[] = "test-hash";
constexpr char kFixedTestTime[] = "2024-01-15T10:00:00Z";
}

class StudentServiceUnitTest : public ::testing::Test {
protected:
    void SetUp() override {
        clock = std::make_unique<FixedClock>(kFixedTestTime);
        studentOneId = userRepo.saveWithPassword("student-one", Role::Student, kTestHash, 0);
        studentTwoId = userRepo.saveWithPassword("student-two", Role::Student, kTestHash, 0);

        deptRepo.departments.push_back(Department(1, "Computer Science"));
        deptRepo.departments.push_back(Department(2, "Mathematics"));

        courseRepo.courses.push_back(Course(kCsCourseId, "CS101", "Intro to Programming", 1));
        courseRepo.courses.push_back(Course(kMathCourseId, "MATH101", "Calculus I", 2));
        courseRepo.enroll(studentOneId, kCsCourseId);
        courseRepo.enroll(studentTwoId, kMathCourseId);
        courseRepo.lecturerNames[kCsCourseId] = {"alice", "bob"};
        courseRepo.lecturerNames[kMathCourseId] = {"carol"};

        assignmentRepo.assignments.push_back(
            Assignment(kEssayAssignmentId, kCsCourseId, "Essay", "2024-01-20T10:00:00Z", 100.0, kLecturerUserId));
        assignmentRepo.assignments.push_back(
            Assignment(kProblemSetAssignmentId, kMathCourseId, "Problem set", "2024-01-22T10:00:00Z", 50.0, kLecturerUserId));

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
    int studentOneId = 0;
    int studentTwoId = 0;
};

TEST_F(StudentServiceUnitTest, ListCoursesReturnsOnlyOwnEnrolments) {
    auto courses = service->listCourses(studentOneId);

    ASSERT_EQ(courses.size(), 1u);
    EXPECT_EQ(courses[0].code, "CS101");
    EXPECT_EQ(courses[0].title, "Intro to Programming");
    EXPECT_EQ(courses[0].department, "Computer Science");
    ASSERT_EQ(courses[0].lecturers.size(), 2u);
    EXPECT_EQ(courses[0].lecturers[0], "alice");
    EXPECT_EQ(courses[0].lecturers[1], "bob");
}

TEST_F(StudentServiceUnitTest, ListCoursesDoesNotIncludeOtherStudentsCourses) {
    auto student1 = service->listCourses(studentOneId);
    auto student2 = service->listCourses(studentTwoId);

    ASSERT_EQ(student1.size(), 1u);
    ASSERT_EQ(student2.size(), 1u);
    EXPECT_EQ(student1[0].code, "CS101");
    EXPECT_EQ(student2[0].code, "MATH101");
}

TEST_F(StudentServiceUnitTest, ListCoursesEmptyWhenNotEnrolled) {
    const int notEnrolledStudentId = userRepo.saveWithPassword("student-unenrolled", Role::Student, kTestHash, 0);
    auto courses = service->listCourses(notEnrolledStudentId);
    EXPECT_TRUE(courses.empty());
}

TEST_F(StudentServiceUnitTest, EffectiveDeadlineWithoutExtensionIsAssignmentDeadline) {
    auto courses = service->listCourses(studentOneId);
    ASSERT_EQ(courses[0].assignments.size(), 1u);
    EXPECT_FALSE(courses[0].assignments[0].hasExtension);
    EXPECT_EQ(courses[0].assignments[0].effectiveDeadline, "2024-01-20T10:00:00Z");
    EXPECT_EQ(courses[0].assignments[0].state, "NotSubmitted");
    EXPECT_DOUBLE_EQ(courses[0].assignments[0].maxMark, 100.0);
}

TEST_F(StudentServiceUnitTest, EffectiveDeadlineUsesStudentExtension) {
    extensionRepo.extensions.push_back(
        Extension(1, kEssayAssignmentId, studentOneId, "2024-01-25T10:00:00Z", "Medical", kLecturerUserId));

    auto courses = service->listCourses(studentOneId);
    ASSERT_EQ(courses[0].assignments.size(), 1u);
    EXPECT_TRUE(courses[0].assignments[0].hasExtension);
    EXPECT_EQ(courses[0].assignments[0].effectiveDeadline, "2024-01-25T10:00:00Z");
}

TEST_F(StudentServiceUnitTest, ExtensionForAnotherStudentIsIgnored) {
    extensionRepo.extensions.push_back(
        Extension(1, kEssayAssignmentId, studentTwoId, "2024-01-25T10:00:00Z", "Medical", kLecturerUserId));

    auto courses = service->listCourses(studentOneId);
    ASSERT_EQ(courses[0].assignments.size(), 1u);
    EXPECT_FALSE(courses[0].assignments[0].hasExtension);
    EXPECT_EQ(courses[0].assignments[0].effectiveDeadline, "2024-01-20T10:00:00Z");
}

TEST_F(StudentServiceUnitTest, AssignmentStateReflectsOwnSubmissionOnly) {
    Submission other(1, kEssayAssignmentId, studentTwoId, SubmissionState::Submitted);
    other.setFirstSubmittedAt("2024-01-14T10:00:00Z");
    submissionRepo.submissions.push_back(other);

    Submission own(2, kEssayAssignmentId, studentOneId, SubmissionState::Late);
    own.setFirstSubmittedAt("2024-01-21T10:00:00Z");
    submissionRepo.submissions.push_back(own);

    auto courses = service->listCourses(studentOneId);
    ASSERT_EQ(courses[0].assignments.size(), 1u);
    EXPECT_EQ(courses[0].assignments[0].state, "Late");
}
