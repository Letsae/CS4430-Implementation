#include <gtest/gtest.h>

#include "application/AdminService.h"
#include "application/AssignmentService.h"
#include "application/NotificationService.h"
#include "application/StudentService.h"
#include "data/AssignmentRepository.h"
#include "data/CourseRepository.h"
#include "data/DepartmentRepository.h"
#include "data/EnrolmentRepository.h"
#include "data/ExtensionRepository.h"
#include "data/NotificationRepository.h"
#include "data/SubmissionRepository.h"
#include "data/SubmissionVersionRepository.h"
#include "data/UserRepository.h"
#include "domain/LmsException.h"
#include "security/BcryptPasswordHasher.h"
#include "support/SchemaPath.h"
#include "support/TestData.h"
#include "util/Clock.h"

#include <ctime>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>

using namespace Lms;

namespace {
std::string localMinute(std::chrono::system_clock::time_point point) {
    const std::time_t value = std::chrono::system_clock::to_time_t(point);
    const std::tm* local = std::localtime(&value);
    std::ostringstream output;
    output << std::put_time(local, "%Y-%m-%d %H:%M");
    return output.str();
}

class FailingAdminNotificationRepository : public INotificationRepository {
public:
    std::unique_ptr<Notification> getById(int) override { throw NotFoundException("missing"); }
    std::vector<Notification> getAll() override { return {}; }
    int save(const Notification&) override { throw DatabaseException("injected notification failure"); }
    void remove(int) override {}
    bool exists(int) override { return false; }
};
}

class AdminAssignmentIntegrationTest : public ::testing::Test {
protected:
    AdminAssignmentIntegrationTest()
        : db(":memory:"), clock("2030-01-02T12:00:00Z"), users(db), departments(db), courses(db),
          assignments(db), submissions(db), versions(db), extensions(db), notifications(db),
          enrolments(db, courses, users), hasher(4) {
        std::ifstream schema(TestSupport::schemaPath());
        if (!schema.is_open()) throw std::runtime_error("Cannot open configured schema file");
        std::stringstream contents;
        contents << schema.rdbuf();
        db.execute(contents.str());
        LmsTestData::TestData data(db);
        departmentId = data.makeDepartment();
        otherDepartmentId = data.makeDepartment();
        courseId = data.makeCourse(departmentId);
        adminId = data.makeUser(Role::Administrator);
        lecturerId = data.makeUser(Role::Lecturer, departmentId);
        otherLecturerId = data.makeUser(Role::Lecturer, otherDepartmentId);
        studentId = data.makeUser(Role::Student, departmentId);
        otherStudentId = data.makeUser(Role::Student, departmentId);
        admin = users.getById(adminId);
        lecturer = users.getById(lecturerId);
        enrolments.enroll(courseId, studentId);
        observer = std::make_shared<NotificationService>(enrolments, notifications, clock);
        events.subscribe(observer);
        service = std::make_unique<AdminService>(users, departments, courses, enrolments, hasher, db, events);
    }

    Database db;
    FixedClock clock;
    SqliteUserRepository users;
    SqliteDepartmentRepository departments;
    SqliteCourseRepository courses;
    SqliteAssignmentRepository assignments;
    SqliteSubmissionRepository submissions;
    SqliteSubmissionVersionRepository versions;
    SqliteExtensionRepository extensions;
    SqliteNotificationRepository notifications;
    SqliteEnrolmentRepository enrolments;
    BcryptPasswordHasher hasher;
    EventPublisher events;
    std::shared_ptr<NotificationService> observer;
    std::unique_ptr<AdminService> service;
    std::unique_ptr<User> admin;
    std::unique_ptr<User> lecturer;
    int departmentId = 0;
    int otherDepartmentId = 0;
    int courseId = 0;
    int adminId = 0;
    int lecturerId = 0;
    int otherLecturerId = 0;
    int studentId = 0;
    int otherStudentId = 0;
};

TEST_F(AdminAssignmentIntegrationTest, PersistsAssignmentAndNotifiesTheLecturer) {
    service->assignLecturerToCourse(*admin, courseId, lecturerId);
    const auto assigned = courses.getByLecturer(lecturerId);
    ASSERT_EQ(assigned.size(), 1u);
    EXPECT_EQ(assigned[0].getId(), courseId);
    const auto notices = notifications.getAll();
    ASSERT_EQ(notices.size(), 1u);
    EXPECT_EQ(notices[0].getUserId(), lecturerId);
    EXPECT_NE(notices[0].getMessage().find(assigned[0].getCode()), std::string::npos);
}

TEST_F(AdminAssignmentIntegrationTest, NotificationFailureRollsBackLecturerAssignment) {
    FailingAdminNotificationRepository failingNotifications;
    EventPublisher failingEvents;
    auto failingObserver = std::make_shared<NotificationService>(enrolments, failingNotifications, clock);
    failingEvents.subscribe(failingObserver);
    AdminService failingService(users, departments, courses, enrolments, hasher, db, failingEvents);
    EXPECT_THROW(failingService.assignLecturerToCourse(*admin, courseId, lecturerId), DatabaseException);
    EXPECT_TRUE(courses.getByLecturer(lecturerId).empty());
    EXPECT_FALSE(db.isInTransaction());
}

TEST_F(AdminAssignmentIntegrationTest, AssignmentEnablementFlowsToEnrolledStudentWithoutStudentCodeChanges) {
    AssignmentService assignmentService(db, courses, assignments, events, clock);
    const std::string localDeadline = localMinute(clock.nowTimePoint() + std::chrono::hours(24));
    EXPECT_THROW(assignmentService.createAssignment(*lecturer, courseId, "Integration work", localDeadline, 50), AuthorisationException);

    service->assignLecturerToCourse(*admin, courseId, lecturerId);
    const auto created = assignmentService.createAssignment(*lecturer, courseId, "Integration work", localDeadline, 50);

    StudentService studentService(courses, assignments, submissions, versions, extensions, departments, users, clock);
    const auto enrolledCourses = studentService.listCourses(studentId);
    ASSERT_EQ(enrolledCourses.size(), 1u);
    ASSERT_EQ(enrolledCourses[0].assignments.size(), 1u);
    EXPECT_EQ(enrolledCourses[0].assignments[0].id, created.assignment.getId());
    EXPECT_EQ(enrolledCourses[0].assignments[0].effectiveDeadline, created.assignment.getDeadline());
    EXPECT_TRUE(studentService.listCourses(otherStudentId).empty());
    EXPECT_NO_THROW(studentService.submit(studentId, created.assignment.getId(), "submission content"));
    EXPECT_THROW(studentService.submit(otherStudentId, created.assignment.getId(), "submission content"), AuthorisationException);
    EXPECT_THROW(assignmentService.createAssignment(*users.getById(otherLecturerId), courseId, "Unauthorized", localDeadline, 10), AuthorisationException);
}

TEST_F(AdminAssignmentIntegrationTest, ParameterizedCourseTextIsStoredLiterally) {
    const std::string title = "'; DROP TABLE course;--";
    const int id = courses.save(Course(0, "SAFE-INJECTION-COURSE", title, departmentId));
    EXPECT_EQ(courses.getById(id)->getTitle(), title);
    EXPECT_NO_THROW(courses.getById(courseId));
}
