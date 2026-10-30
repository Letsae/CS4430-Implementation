#include <gtest/gtest.h>

#include "application/MarkingService.h"
#include "data/AssignmentRepository.h"
#include "data/CourseRepository.h"
#include "data/ExtensionRepository.h"
#include "data/MarkAuditRepository.h"
#include "data/SubmissionRepository.h"
#include "data/SubmissionVersionRepository.h"
#include "data/UserRepository.h"
#include "domain/LmsException.h"
#include "support/SchemaPath.h"
#include "support/TestData.h"

#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>

using namespace Lms;

namespace {
constexpr char kMarkingTime[] = "2035-06-02T12:00:00Z";
constexpr char kLateSubmittedAt[] = "2035-06-01T12:00:01Z";
constexpr double kInitialRawMark = 80.0;
constexpr double kRevisedRawMark = 90.0;
constexpr double kAssignmentMaxMark = 100.0;
constexpr char kFeedback[] = "Clear explanation";
constexpr char kRevisionReason[] = "Corrected omitted section";

class MarkEventCounter : public IObserver {
public:
    int marksChanged = 0;
    void onEvent(const Event& event) override {
        if (event.getType() == "MarkChanged") ++marksChanged;
    }
};
}

class MarkingServiceIntegrationTest : public ::testing::Test {
protected:
    MarkingServiceIntegrationTest()
        : database(":memory:"), clock(kMarkingTime), users(database), courses(database),
          assignments(database), submissions(database), versions(database), extensions(database),
          audit(database) {
        std::ifstream schema(TestSupport::schemaPath());
        if (!schema) throw std::runtime_error("Cannot open configured schema file");
        std::stringstream sql;
        sql << schema.rdbuf();
        database.execute(sql.str());
        LmsTestData::TestData data(database);
        const int departmentId = data.makeDepartment();
        lecturerId = data.makeUser(Role::Lecturer, departmentId);
        otherLecturerId = data.makeUser(Role::Lecturer, departmentId);
        studentId = data.makeUser(Role::Student, departmentId);
        const int otherCourseDepartmentId = data.makeDepartment();
        otherStudentId = data.makeUser(Role::Student, otherCourseDepartmentId);
        courseId = data.makeCourse(departmentId);
        otherCourseId = data.makeCourse(otherCourseDepartmentId);
        assignmentId = data.makeAssignment(courseId, lecturerId);
        otherAssignmentId = data.makeAssignment(otherCourseId, lecturerId);
        courses.assignLecturer(courseId, lecturerId);
        courses.assignLecturer(otherCourseId, otherLecturerId);
        const int id = data.makeSubmission(assignmentId, studentId);
        auto submission = submissions.getById(id);
        submission->setState(SubmissionState::Late);
        submission->setFirstSubmittedAt(kLateSubmittedAt);
        submissions.save(*submission);
        submissionId = id;
        lecturer = users.getById(lecturerId);
        otherLecturer = users.getById(otherLecturerId);
        student = users.getById(studentId);
        observer = std::make_shared<MarkEventCounter>();
        events.subscribe(observer);
        service = std::make_unique<MarkingService>(database, submissions, audit, assignments,
            courses, versions, users, extensions, penalty, events, clock);
    }

    Database database;
    FixedClock clock;
    SqliteUserRepository users;
    SqliteCourseRepository courses;
    SqliteAssignmentRepository assignments;
    SqliteSubmissionRepository submissions;
    SqliteSubmissionVersionRepository versions;
    SqliteExtensionRepository extensions;
    SqliteMarkAuditRepository audit;
    EventPublisher events;
    StandardLatePenalty penalty;
    std::shared_ptr<MarkEventCounter> observer;
    std::unique_ptr<MarkingService> service;
    std::unique_ptr<User> lecturer;
    std::unique_ptr<User> otherLecturer;
    std::unique_ptr<User> student;
    int lecturerId = 0;
    int otherLecturerId = 0;
    int studentId = 0;
    int otherStudentId = 0;
    int courseId = 0;
    int otherCourseId = 0;
    int assignmentId = 0;
    int otherAssignmentId = 0;
    int submissionId = 0;
};

TEST_F(MarkingServiceIntegrationTest, MarksLateSubmissionAndAuditsRevision) {
    service->markSubmission(*lecturer, submissionId, kInitialRawMark, kFeedback,
                            MarkingService::InitialMarkReason);
    auto marked = submissions.getById(submissionId);
    EXPECT_EQ(marked->getState(), SubmissionState::Marked);
    EXPECT_DOUBLE_EQ(marked->getPenaltyPercent(), 10.0);
    EXPECT_DOUBLE_EQ(marked->getFinalMark(), 70.0);
    EXPECT_EQ(observer->marksChanged, 1);

    service->markSubmission(*lecturer, submissionId, kRevisedRawMark, kFeedback, kRevisionReason);
    marked = submissions.getById(submissionId);
    EXPECT_DOUBLE_EQ(marked->getFinalMark(), 80.0);
    const auto entries = audit.getBySubmission(submissionId);
    ASSERT_EQ(entries.size(), 2u);
    EXPECT_DOUBLE_EQ(entries[0].getOldValue(), 0.0);
    EXPECT_DOUBLE_EQ(entries[0].getNewValue(), 70.0);
    EXPECT_EQ(entries[0].getReason(), MarkingService::InitialMarkReason);
    EXPECT_DOUBLE_EQ(entries[1].getOldValue(), 70.0);
    EXPECT_DOUBLE_EQ(entries[1].getNewValue(), 80.0);
    EXPECT_EQ(entries[1].getReason(), kRevisionReason);
    EXPECT_EQ(observer->marksChanged, 2);
}

TEST_F(MarkingServiceIntegrationTest, EnforcesRoleCourseAndValueBoundariesWithoutEvents) {
    EXPECT_THROW(service->markSubmission(*student, submissionId, kInitialRawMark,
        kFeedback, MarkingService::InitialMarkReason), AuthorisationException);
    EXPECT_THROW(service->markSubmission(*otherLecturer, submissionId, kInitialRawMark,
        kFeedback, MarkingService::InitialMarkReason), AuthorisationException);
    EXPECT_THROW(service->markSubmission(*lecturer, submissionId, -0.01,
        kFeedback, MarkingService::InitialMarkReason), ValidationException);
    EXPECT_THROW(service->markSubmission(*lecturer, submissionId, kAssignmentMaxMark + 0.01,
        kFeedback, MarkingService::InitialMarkReason), ValidationException);
    EXPECT_THROW(service->markSubmission(*lecturer, submissionId, kInitialRawMark,
        "  ", MarkingService::InitialMarkReason), ValidationException);
    EXPECT_EQ(observer->marksChanged, 0);
    EXPECT_TRUE(audit.getBySubmission(submissionId).empty());

    EXPECT_NO_THROW(service->markSubmission(*lecturer, submissionId, 0.0,
        kFeedback, MarkingService::InitialMarkReason));
}
