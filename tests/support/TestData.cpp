#include "TestData.h"

#include "data/AssignmentRepository.h"
#include "data/CourseRepository.h"
#include "data/DepartmentRepository.h"
#include "data/EnrolmentRepository.h"
#include "data/SubmissionRepository.h"
#include "data/UserRepository.h"
#include "security/BcryptPasswordHasher.h"

#include <atomic>
#include <string>

namespace LmsTestData {
using namespace Lms;
namespace {
std::atomic<int> valueCounter{0};
constexpr char kFixturePassword[] = "fixture-password-value";
constexpr char kFixtureDeadline[] = "2035-06-01T12:00:00Z";
constexpr char kFixtureSubmittedAt[] = "2035-05-31T12:00:00Z";
constexpr double kFixtureMaximumMark = 100.0;
}

TestData::TestData(Database& database) : m_database(database) {}

int TestData::nextValue() { return ++valueCounter; }

int TestData::makeDepartment() {
    SqliteDepartmentRepository departments(m_database);
    const int value = nextValue();
    return departments.save(Department(0, "fixture-department-" + std::to_string(value)));
}

int TestData::makeUser(Role role, int departmentId) {
    if (role == Role::Lecturer || role == Role::Student || role == Role::HeadOfDepartment) {
        if (departmentId <= 0) departmentId = makeDepartment();
    }
    SqliteUserRepository users(m_database);
    BcryptPasswordHasher hasher(4);
    const int value = nextValue();
    const std::string username = "fixture-user-" + std::to_string(value);
    return users.saveWithPassword(username, role, hasher.hashPassword(kFixturePassword), departmentId);
}

int TestData::makeCourse(int departmentId) {
    SqliteCourseRepository courses(m_database);
    const int value = nextValue();
    const std::string suffix = std::to_string(value);
    return courses.save(Course(0, "FIX" + suffix, "Fixture course " + suffix, departmentId));
}

void TestData::makeEnrolment(int courseId, int studentId) {
    SqliteCourseRepository courses(m_database);
    SqliteUserRepository users(m_database);
    SqliteEnrolmentRepository enrolments(m_database, courses, users);
    enrolments.enroll(courseId, studentId);
}

int TestData::makeAssignment(int courseId, int lecturerId) {
    SqliteAssignmentRepository assignments(m_database);
    const int value = nextValue();
    return assignments.save(Assignment(0, courseId, "Fixture assignment " + std::to_string(value),
                                       kFixtureDeadline, kFixtureMaximumMark, lecturerId));
}

int TestData::makeSubmission(int assignmentId, int studentId) {
    SqliteSubmissionRepository submissions(m_database);
    Submission submission(0, assignmentId, studentId, SubmissionState::Submitted);
    submission.setFirstSubmittedAt(kFixtureSubmittedAt);
    return submissions.save(submission);
}

} // namespace LmsTestData
