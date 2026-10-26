#include <gtest/gtest.h>
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
#include "domain/Observer.h"
#include "support/SchemaPath.h"
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
    std::time_t value = std::chrono::system_clock::to_time_t(point);
    std::tm* local = std::localtime(&value);
    std::ostringstream output;
    output << std::put_time(local, "%Y-%m-%d %H:%M");
    return output.str();
}

class FailingNotificationRepository : public INotificationRepository {
public:
    std::unique_ptr<Notification> getById(int) override { throw NotFoundException("missing"); }
    std::vector<Notification> getAll() override { return {}; }
    int save(const Notification&) override { throw DatabaseException("injected notification failure"); }
    void remove(int) override {}
    bool exists(int) override { return false; }
};
}

class AssignmentCreationIntegrationTest : public ::testing::Test {
protected:
    AssignmentCreationIntegrationTest()
        : db(":memory:"), clock("2030-01-02T12:00:00Z"), users(db), departments(db), courses(db),
          assignments(db), submissions(db), versions(db), extensions(db), notifications(db),
          enrolments(db, courses, users) {
        std::ifstream schema(TestSupport::schemaPath());
        if (!schema.is_open()) throw std::runtime_error("Cannot open configured schema file");
        std::stringstream contents;
        contents << schema.rdbuf();
        db.execute(contents.str());

        departmentId = departments.save(Department(0, "Computing"));
        lecturerId = users.saveWithPassword("lecturer", Role::Lecturer, "hash", departmentId);
        studentId = users.saveWithPassword("enrolled", Role::Student, "hash", departmentId);
        otherStudentId = users.saveWithPassword("unenrolled", Role::Student, "hash", departmentId);
        courseId = courses.save(Course(0, "CS101", "Computer Science", departmentId));
        courses.assignLecturer(courseId, lecturerId);
        enrolments.enroll(courseId, studentId);
        lecturer = std::make_unique<Lecturer>(lecturerId, "lecturer", departmentId);
        auto observer = std::make_shared<NotificationService>(enrolments, notifications, clock);
        publisher.subscribe(observer);
        observerKeepAlive = observer;
        service = std::make_unique<AssignmentService>(db, courses, assignments, publisher, clock);
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
    EventPublisher publisher;
    std::shared_ptr<NotificationService> observerKeepAlive;
    std::unique_ptr<AssignmentService> service;
    std::unique_ptr<Lecturer> lecturer;
    int departmentId = 0;
    int lecturerId = 0;
    int studentId = 0;
    int otherStudentId = 0;
    int courseId = 0;
};

TEST_F(AssignmentCreationIntegrationTest, PersistsNotifiesAndIsVisibleAndSubmittableToEnrolledStudent) {
    const std::string localDeadline = localMinute(clock.nowTimePoint() + std::chrono::hours(24));
    auto created = service->createAssignment(*lecturer, courseId, "Research task", localDeadline, 75.50);

    auto stored = assignments.getById(created.assignment.getId());
    EXPECT_EQ(stored->getCourseId(), courseId);
    EXPECT_EQ(stored->getCreatedBy(), lecturerId);
    EXPECT_EQ(stored->getDeadline(), created.assignment.getDeadline());
    EXPECT_EQ(stored->getDeadline().back(), 'Z');

    const auto noticeRows = notifications.getAll();
    ASSERT_EQ(noticeRows.size(), 1u);
    EXPECT_EQ(noticeRows[0].getUserId(), studentId);
    EXPECT_EQ(noticeRows[0].getMessage(), "New assignment: Research task, due " + localDeadline);

    StudentService studentService(courses, assignments, submissions, versions, extensions, departments, users, clock);
    auto studentCourses = studentService.listCourses(studentId);
    ASSERT_EQ(studentCourses.size(), 1u);
    ASSERT_EQ(studentCourses[0].assignments.size(), 1u);
    EXPECT_EQ(studentCourses[0].assignments[0].title, "Research task");
    EXPECT_EQ(studentCourses[0].assignments[0].effectiveDeadline, stored->getDeadline());
    studentService.submit(studentId, stored->getId(), "My work");
    EXPECT_NO_THROW(submissions.getByAssignmentAndStudent(stored->getId(), studentId));

    EXPECT_TRUE(studentService.listCourses(otherStudentId).empty());
}

TEST_F(AssignmentCreationIntegrationTest, StoresSqlInjectionTextLiterallyAndAssignmentTableSurvives) {
    const std::string title = "'; DROP TABLE assignment;--";
    const auto localDeadline = localMinute(clock.nowTimePoint() + std::chrono::hours(48));
    auto created = service->createAssignment(*lecturer, courseId, title, localDeadline, 10);
    EXPECT_EQ(assignments.getById(created.assignment.getId())->getTitle(), title);
    EXPECT_EQ(assignments.getByCourse(courseId).size(), 1u);
}

TEST_F(AssignmentCreationIntegrationTest, NotificationFailureRollsBackAssignment) {
    FailingNotificationRepository failingNotifications;
    EventPublisher failingPublisher;
    auto failingObserver = std::make_shared<NotificationService>(enrolments, failingNotifications, clock);
    failingPublisher.subscribe(failingObserver);
    AssignmentService failingService(db, courses, assignments, failingPublisher, clock);
    EXPECT_THROW(failingService.createAssignment(*lecturer, courseId, "Rollback", 
        localMinute(clock.nowTimePoint() + std::chrono::hours(72)), 10), DatabaseException);
    EXPECT_TRUE(assignments.getByCourse(courseId).empty());
}
