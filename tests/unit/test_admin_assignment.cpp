#include <gtest/gtest.h>
#include "application/AdminService.h"
#include "domain/LmsException.h"
#include "fakes/FakeRepositories.h"

using namespace Lms;

namespace {
constexpr int kDepartmentId = 31;
constexpr int kOtherDepartmentId = 32;
constexpr int kCourseId = 41;
constexpr int kOtherCourseId = 42;
constexpr int kStudentId = 90;
constexpr int kLecturerId = 91;
constexpr int kHeadOfDepartmentId = 92;

class AssignmentEventCounter : public IObserver {
public:
    int count = 0;
    void onEvent(const Event& event) override { if (event.getType() == "LecturerAssigned") ++count; }
};
}

class AdminAssignmentServiceTest : public ::testing::Test {
protected:
    AdminAssignmentServiceTest()
        : enrolments(courses), service(users, departments, courses, enrolments, hasher, database, events),
          observer(std::make_shared<AssignmentEventCounter>()) {
        departments.departments.emplace_back(kDepartmentId, "Department One");
        departments.departments.emplace_back(kOtherDepartmentId, "Department Two");
        courses.courses.emplace_back(kCourseId, "COURSE-A", "Course A", kDepartmentId);
        courses.courses.emplace_back(kOtherCourseId, "COURSE-B", "Course B", kOtherDepartmentId);
        administratorId = users.saveWithPassword("admin", Role::Administrator, "hash", 0);
        lecturerId = users.saveWithPassword("lecturer", Role::Lecturer, "hash", kDepartmentId);
        events.subscribe(observer);
    }

    Database database{":memory:"};
    Lms::Test::FakeUserRepository users;
    Lms::Test::FakeDepartmentRepository departments;
    Lms::Test::FakeCourseRepository courses;
    Lms::Test::FakeEnrolmentRepository enrolments;
    Lms::Test::FakePasswordHasher hasher;
    EventPublisher events;
    AdminService service;
    std::shared_ptr<AssignmentEventCounter> observer;
    int administratorId = 0;
    int lecturerId = 0;
    Administrator administrator{0, "admin"};
};

TEST_F(AdminAssignmentServiceTest, AssignsLecturerAndPublishesExactlyOnce) {
    service.assignLecturerToCourse(administrator, kCourseId, lecturerId);
    ASSERT_EQ(courses.getByLecturer(lecturerId).size(), 1u);
    EXPECT_EQ(courses.getByLecturer(lecturerId)[0].getId(), kCourseId);
    EXPECT_EQ(observer->count, 1);
    EXPECT_FALSE(database.isInTransaction());
}

TEST_F(AdminAssignmentServiceTest, RejectsNonAdministratorWithoutPublishing) {
    Student student(kStudentId, "student");
    Lecturer lecturer(kLecturerId, "lecturer", kDepartmentId);
    HeadOfDepartment hod(kHeadOfDepartmentId, "hod", kDepartmentId);
    EXPECT_THROW(service.assignLecturerToCourse(student, kCourseId, lecturerId), AuthorisationException);
    EXPECT_THROW(service.assignLecturerToCourse(lecturer, kCourseId, lecturerId), AuthorisationException);
    EXPECT_THROW(service.assignLecturerToCourse(hod, kCourseId, lecturerId), AuthorisationException);
    EXPECT_EQ(observer->count, 0);
}

TEST_F(AdminAssignmentServiceTest, RejectsUnknownCourseOrUser) {
    constexpr int kUnknownEntityId = 404;
    EXPECT_THROW(service.assignLecturerToCourse(administrator, kUnknownEntityId, lecturerId), NotFoundException);
    EXPECT_THROW(service.assignLecturerToCourse(administrator, kCourseId, kUnknownEntityId), NotFoundException);
    EXPECT_EQ(observer->count, 0);
}

TEST_F(AdminAssignmentServiceTest, RejectsNonLecturerInactiveLecturerAndDepartmentMismatch) {
    const int studentId = users.saveWithPassword("student", Role::Student, "hash", kDepartmentId);
    EXPECT_THROW(service.assignLecturerToCourse(administrator, kCourseId, studentId), ValidationException);
    users.inactiveIds.push_back(lecturerId);
    EXPECT_THROW(service.assignLecturerToCourse(administrator, 41, lecturerId), ValidationException);
    users.inactiveIds.clear();
    const int otherLecturer = users.saveWithPassword("other", Role::Lecturer, "hash", kOtherDepartmentId);
    EXPECT_THROW(service.assignLecturerToCourse(administrator, kCourseId, otherLecturer), ValidationException);
    EXPECT_EQ(observer->count, 0);
}

TEST_F(AdminAssignmentServiceTest, RejectsDuplicateWithoutPublishingAgain) {
    courses.lecturerCourseIds[lecturerId].push_back(kCourseId);
    EXPECT_THROW(service.assignLecturerToCourse(administrator, kCourseId, lecturerId), ValidationException);
    EXPECT_EQ(observer->count, 0);
}
