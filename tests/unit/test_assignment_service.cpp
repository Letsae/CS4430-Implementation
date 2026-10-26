#include <gtest/gtest.h>
#include "application/AssignmentService.h"
#include "domain/LmsException.h"
#include "domain/Observer.h"
#include "fakes/FakeRepositories.h"
#include <ctime>
#include <iomanip>
#include <memory>
#include <sstream>

using namespace Lms;

namespace {
class EventCounter : public IObserver {
public:
    int assignmentsCreated = 0;
    void onEvent(const Event& event) override {
        if (event.getType() == "AssignmentCreated") ++assignmentsCreated;
    }
};

std::string localMinute(std::chrono::system_clock::time_point point) {
    std::time_t value = std::chrono::system_clock::to_time_t(point);
    std::tm* local = std::localtime(&value);
    std::ostringstream output;
    output << std::put_time(local, "%Y-%m-%d %H:%M");
    return output.str();
}
}

class AssignmentServiceUnitTest : public ::testing::Test {
protected:
    AssignmentServiceUnitTest()
        : clock("2030-01-02T12:00:00Z"), service(db, courses, assignments, events, clock),
          observer(std::make_shared<EventCounter>()) {
        events.subscribe(observer);
        courses.courses.emplace_back(7, "CS101", "Intro to CS", 1);
        courses.lecturerCourseIds[4] = {7};
    }
    Database db{":memory:"};
    Lms::Test::FakeCourseRepository courses;
    Lms::Test::FakeAssignmentRepository assignments;
    EventPublisher events;
    FixedClock clock;
    AssignmentService service;
    std::shared_ptr<EventCounter> observer;
    Lecturer lecturer{4, "lecturer", 1};
    std::string future = localMinute(clock.nowTimePoint() + std::chrono::hours(24));
};

TEST_F(AssignmentServiceUnitTest, CreatesAssignmentAndPublishesOnce) {
    auto created = service.createAssignment(lecturer, 7, "  Essay  ", future, 100.25);
    EXPECT_EQ(created.assignment.getId(), 1);
    EXPECT_EQ(created.assignment.getTitle(), "Essay");
    EXPECT_EQ(created.assignment.getCourseId(), 7);
    EXPECT_EQ(created.assignment.getCreatedBy(), 4);
    EXPECT_EQ(assignments.assignments.size(), 1u);
    EXPECT_EQ(observer->assignmentsCreated, 1);
}

TEST_F(AssignmentServiceUnitTest, RejectsNonLecturerRoles) {
    Student student(8, "student");
    HeadOfDepartment hod(9, "hod", 1);
    Administrator admin(10, "admin");
    EXPECT_THROW(service.createAssignment(student, 7, "Title", future, 10), AuthorisationException);
    EXPECT_THROW(service.createAssignment(hod, 7, "Title", future, 10), AuthorisationException);
    EXPECT_THROW(service.createAssignment(admin, 7, "Title", future, 10), AuthorisationException);
    EXPECT_EQ(observer->assignmentsCreated, 0);
}

TEST_F(AssignmentServiceUnitTest, RejectsLecturerOfDifferentCourse) {
    courses.courses.emplace_back(8, "MA101", "Math", 1);
    EXPECT_THROW(service.createAssignment(lecturer, 8, "Title", future, 10), AuthorisationException);
    EXPECT_EQ(observer->assignmentsCreated, 0);
}

TEST_F(AssignmentServiceUnitTest, RejectsUnknownCourse) {
    EXPECT_THROW(service.createAssignment(lecturer, 999, "Title", future, 10), NotFoundException);
    EXPECT_EQ(observer->assignmentsCreated, 0);
}

TEST_F(AssignmentServiceUnitTest, RejectsBlankAndOverlongTitles) {
    EXPECT_THROW(service.createAssignment(lecturer, 7, "  \t ", future, 10), ValidationException);
    EXPECT_THROW(service.createAssignment(lecturer, 7, std::string(101, 'x'), future, 10), ValidationException);
}

TEST_F(AssignmentServiceUnitTest, ValidatesMaximumMarkBoundsAndPrecision) {
    EXPECT_THROW(service.createAssignment(lecturer, 7, "A", future, 0), ValidationException);
    EXPECT_THROW(service.createAssignment(lecturer, 7, "B", future, -1), ValidationException);
    EXPECT_THROW(service.createAssignment(lecturer, 7, "C", future, 1000.01), ValidationException);
    EXPECT_THROW(service.createAssignment(lecturer, 7, "D", future, 1.001), ValidationException);
    EXPECT_THROW(service.parseMaximumMark("1.001"), ValidationException);
    EXPECT_THROW(service.parseMaximumMark("1.000"), ValidationException);
    EXPECT_NO_THROW(service.createAssignment(lecturer, 7, "E", future, 1000.0));
}

TEST_F(AssignmentServiceUnitTest, RejectsPastAndEqualDeadlines) {
    const auto now = clock.nowTimePoint();
    EXPECT_THROW(service.createAssignment(lecturer, 7, "Past", localMinute(now - std::chrono::minutes(1)), 10), ValidationException);
    EXPECT_THROW(service.createAssignment(lecturer, 7, "Equal", localMinute(now), 10), ValidationException);
}

TEST_F(AssignmentServiceUnitTest, AcceptsDeadlineOneSecondInFuture) {
    FixedClock lastSecond("2030-01-02T12:00:59Z");
    AssignmentService atBoundary(db, courses, assignments, events, lastSecond);
    const auto nextSecond = localMinute(lastSecond.nowTimePoint() + std::chrono::seconds(1));
    EXPECT_NO_THROW(atBoundary.createAssignment(lecturer, 7, "One second", nextSecond, 10));
}

TEST_F(AssignmentServiceUnitTest, RejectsInvalidDeadlineFormatAndDate) {
    EXPECT_THROW(service.createAssignment(lecturer, 7, "Bad format", "2030/01/03 12:00", 10), ValidationException);
    EXPECT_THROW(service.createAssignment(lecturer, 7, "Bad date", "2030-02-30 12:00", 10), ValidationException);
}

TEST_F(AssignmentServiceUnitTest, RejectsDuplicateCaseInsensitiveTrimmedTitle) {
    assignments.assignments.emplace_back(1, 7, "  Portfolio ", "2030-01-05T12:00:00Z", 10, 4);
    EXPECT_THROW(service.createAssignment(lecturer, 7, "portfolio", future, 10), ValidationException);
    EXPECT_EQ(observer->assignmentsCreated, 0);
}

TEST_F(AssignmentServiceUnitTest, DoesNotPublishOnValidationFailure) {
    EXPECT_THROW(service.createAssignment(lecturer, 7, "", future, 10), ValidationException);
    EXPECT_EQ(observer->assignmentsCreated, 0);
}
