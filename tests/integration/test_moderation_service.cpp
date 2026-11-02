#include <gtest/gtest.h>

#include "application/ModerationService.h"
#include "application/AdminService.h"
#include "application/MarkingService.h"
#include "application/AssignmentService.h"
#include "application/ExtensionService.h"
#include "application/NotificationService.h"
#include "application/StudentService.h"
#include "data/AssignmentRepository.h"
#include "data/CourseRepository.h"
#include "data/DepartmentRepository.h"
#include "data/EnrolmentRepository.h"
#include "data/ExtensionRepository.h"
#include "data/MarkAuditRepository.h"
#include "data/NotificationRepository.h"
#include "data/SubmissionRepository.h"
#include "data/SubmissionVersionRepository.h"
#include "data/UserRepository.h"
#include "domain/LmsException.h"
#include "security/BcryptPasswordHasher.h"
#include "support/SchemaPath.h"
#include "support/TestData.h"

#include <fstream>
#include <algorithm>
#include <memory>
#include <sstream>
#include <stdexcept>

using namespace Lms;

namespace {
constexpr char kModerationTime[] = "2035-05-30T12:00:00Z";
constexpr char kApprovalReason[] = "Reviewed against rubric";
constexpr char kAdjustmentReason[] = "Corrected arithmetic error";
constexpr double kStudentARawMark = 80.0;
constexpr double kStudentBRawMark = 70.0;
constexpr double kAdjustedStudentBRawMark = 60.0;

class FailingNotificationRepository : public INotificationRepository {
public:
    std::unique_ptr<Notification> getById(int) override { throw NotFoundException("missing"); }
    std::vector<Notification> getAll() override { return {}; }
    int save(const Notification&) override { throw DatabaseException("injected notification failure"); }
    void remove(int) override {}
    bool exists(int) override { return false; }
};
}

class ModerationWorkflowIntegrationTest : public ::testing::Test {
protected:
    ModerationWorkflowIntegrationTest()
        : database(":memory:"), clock(kModerationTime), users(database), departments(database),
          courses(database), assignments(database), submissions(database), versions(database),
          extensions(database), audit(database), notifications(database),
          enrolments(database, courses, users), events() {
        std::ifstream schema(TestSupport::schemaPath());
        if (!schema) throw std::runtime_error("Cannot open configured schema file");
        std::stringstream sql;
        sql << schema.rdbuf();
        database.execute(sql.str());

        LmsTestData::TestData data(database);
        departmentId = data.makeDepartment();
        otherDepartmentId = data.makeDepartment();
        hodId = data.makeUser(Role::HeadOfDepartment, departmentId);
        otherHodId = data.makeUser(Role::HeadOfDepartment, otherDepartmentId);
        lecturerId = data.makeUser(Role::Lecturer, departmentId);
        studentAId = data.makeUser(Role::Student, departmentId);
        studentBId = data.makeUser(Role::Student, departmentId);
        otherStudentId = data.makeUser(Role::Student, otherDepartmentId);
        courseId = data.makeCourse(departmentId);
        otherCourseId = data.makeCourse(otherDepartmentId);
        assignmentId = data.makeAssignment(courseId, lecturerId);
        otherAssignmentId = data.makeAssignment(otherCourseId, lecturerId);
        data.makeEnrolment(courseId, studentAId);
        data.makeEnrolment(courseId, studentBId);
        studentA = users.getById(studentAId);
        studentB = users.getById(studentBId);
        hod = users.getById(hodId);
        otherHod = users.getById(otherHodId);
        makeMarkedSubmission(assignmentId, studentAId, kStudentARawMark);
        studentASubmissionId = submissions.getByAssignmentAndStudent(assignmentId, studentAId)->getId();
        makeMarkedSubmission(assignmentId, studentBId, kStudentBRawMark);
        studentBSubmissionId = submissions.getByAssignmentAndStudent(assignmentId, studentBId)->getId();
        makeMarkedSubmission(otherAssignmentId, otherStudentId, kStudentARawMark);
        observer = std::make_shared<NotificationService>(enrolments, notifications, clock);
        events.subscribe(observer);
        service = std::make_unique<ModerationService>(database, submissions, audit, courses,
            assignments, users, events, clock);
        studentService = std::make_unique<StudentService>(courses, assignments, submissions,
            versions, extensions, departments, users, clock);
    }

    void makeMarkedSubmission(int itemAssignmentId, int studentId, double rawMark) {
        LmsTestData::TestData data(database);
        const int id = data.makeSubmission(itemAssignmentId, studentId);
        auto submission = submissions.getById(id);
        submission->setState(SubmissionState::Marked);
        submission->setRawMark(rawMark);
        submission->setFinalMark(rawMark);
        submission->setFeedback("Reviewed work");
        submissions.save(*submission);
    }

    Database database;
    FixedClock clock;
    SqliteUserRepository users;
    SqliteDepartmentRepository departments;
    SqliteCourseRepository courses;
    SqliteAssignmentRepository assignments;
    SqliteSubmissionRepository submissions;
    SqliteSubmissionVersionRepository versions;
    SqliteExtensionRepository extensions;
    SqliteMarkAuditRepository audit;
    SqliteNotificationRepository notifications;
    SqliteEnrolmentRepository enrolments;
    EventPublisher events;
    std::shared_ptr<NotificationService> observer;
    std::unique_ptr<ModerationService> service;
    std::unique_ptr<StudentService> studentService;
    std::unique_ptr<User> hod;
    std::unique_ptr<User> otherHod;
    std::unique_ptr<User> studentA;
    std::unique_ptr<User> studentB;
    int departmentId = 0;
    int otherDepartmentId = 0;
    int hodId = 0;
    int otherHodId = 0;
    int lecturerId = 0;
    int studentAId = 0;
    int studentBId = 0;
    int otherStudentId = 0;
    int courseId = 0;
    int otherCourseId = 0;
    int assignmentId = 0;
    int otherAssignmentId = 0;
    int studentASubmissionId = 0;
    int studentBSubmissionId = 0;
};

TEST_F(ModerationWorkflowIntegrationTest, ModeratesScopesAuditsAndReleasesToStudents) {
    ASSERT_EQ(service->reviewMarkedSubmissions(*hod).size(), 2u);
    EXPECT_THROW(service->reviewMarkedSubmissions(*studentA), AuthorisationException);
    EXPECT_THROW(service->auditForAssignment(*hod, otherAssignmentId), AuthorisationException);

    service->approveSubmission(*hod, studentASubmissionId, kApprovalReason);
    service->adjustSubmissionMark(*hod, studentBSubmissionId, kAdjustedStudentBRawMark, kAdjustmentReason);
    EXPECT_FALSE(studentService->listSubmissions(studentAId)[0].markReleased);
    EXPECT_FALSE(studentService->listSubmissions(studentBId)[0].markReleased);

    const auto reviews = service->reviewMarkedSubmissions(*hod);
    EXPECT_TRUE(reviews.empty());
    EXPECT_THROW(service->releaseAssignmentMarks(*otherHod, assignmentId), AuthorisationException);
    service->releaseAssignmentMarks(*hod, assignmentId);

    const auto studentAResults = studentService->listSubmissions(studentAId);
    const auto studentBResults = studentService->listSubmissions(studentBId);
    ASSERT_EQ(studentAResults.size(), 1u);
    ASSERT_EQ(studentBResults.size(), 1u);
    EXPECT_TRUE(studentAResults[0].markReleased);
    EXPECT_DOUBLE_EQ(studentAResults[0].mark, kStudentARawMark);
    EXPECT_EQ(studentAResults[0].feedback, "Reviewed work");
    EXPECT_TRUE(studentBResults[0].markReleased);
    EXPECT_DOUBLE_EQ(studentBResults[0].mark, kAdjustedStudentBRawMark);

    const auto entries = service->auditForAssignment(*hod, assignmentId);
    ASSERT_EQ(entries.size(), 2u);
    EXPECT_EQ(entries[0].getChangedBy(), hodId);
    EXPECT_DOUBLE_EQ(entries[0].getOldValue(), kStudentARawMark);
    EXPECT_DOUBLE_EQ(entries[0].getNewValue(), kStudentARawMark);
    EXPECT_EQ(entries[0].getReason(), std::string("APPROVED: ") + kApprovalReason);
    EXPECT_EQ(entries[1].getChangedBy(), hodId);
    EXPECT_DOUBLE_EQ(entries[1].getOldValue(), kStudentBRawMark);
    EXPECT_DOUBLE_EQ(entries[1].getNewValue(), kAdjustedStudentBRawMark);
    EXPECT_EQ(entries[1].getReason(), kAdjustmentReason);
    EXPECT_EQ(notifications.getAll().size(), 2u);
    EXPECT_THROW(database.execute("UPDATE mark_audit SET reason = 'changed';"), DatabaseException);
    EXPECT_THROW(database.execute("DELETE FROM mark_audit;"), DatabaseException);
}

TEST_F(ModerationWorkflowIntegrationTest, ReleaseRollsBackWhenStudentNotificationFails) {
    service->approveSubmission(*hod, studentASubmissionId, kApprovalReason);
    FailingNotificationRepository failingNotifications;
    EventPublisher failingEvents;
    auto failingObserver = std::make_shared<NotificationService>(enrolments, failingNotifications, clock);
    failingEvents.subscribe(failingObserver);
    ModerationService failingService(database, submissions, audit, courses, assignments,
                                     users, failingEvents, clock);
    EXPECT_THROW(failingService.releaseAssignmentMarks(*hod, assignmentId), DatabaseException);
    EXPECT_EQ(submissions.getById(studentASubmissionId)->getState(), SubmissionState::Moderated);
    EXPECT_FALSE(database.isInTransaction());
}

TEST_F(ModerationWorkflowIntegrationTest, EndToEndWorkflow) {
    constexpr char kWorkflowDepartment[] = "workflow-department";
    constexpr char kWorkflowHodName[] = "workflow-hod";
    constexpr char kWorkflowLecturerName[] = "workflow-lecturer";
    constexpr char kWorkflowStudentAName[] = "workflow-student-a";
    constexpr char kWorkflowStudentBName[] = "workflow-student-b";
    constexpr char kWorkflowStudentCName[] = "workflow-student-c";
    constexpr char kWorkflowCourseCode[] = "WORKFLOW-101";
    constexpr char kWorkflowCourseTitle[] = "Workflow course";
    constexpr char kWorkflowPassword[] = "workflow-test-password";
    constexpr char kWorkflowAssignmentTitle[] = "End-to-end assignment";
    constexpr char kWorkflowDeadlineLocal[] = "2035-06-01 14:00";
    constexpr char kLateTime[] = "2035-06-02T12:00:00Z";
    constexpr char kExtensionDeadline[] = "2035-06-05T12:00:00Z";
    constexpr char kExtensionReason[] = "Approved access arrangement";
    constexpr char kModerationReason[] = "Checked against rubric";
    constexpr char kAdjustmentReason[] = "Corrected calculation";
    constexpr char kFeedback[] = "Good reasoning";
    constexpr double kMaximumMark = 100.0;
    constexpr double kStudentARaw = 80.0;
    constexpr double kStudentBRaw = 70.0;
    constexpr double kStudentBAdjusted = 60.0;

    // Create every account, department, course, assignment, enrolment, and
    // extension through the application services using generated repository IDs.
    LmsTestData::TestData data(database);
    BcryptPasswordHasher testHasher(4);
    const int workflowAdminId = data.makeUser(Role::Administrator);
    auto workflowAdmin = users.getById(workflowAdminId);
    AdminService adminService(users, departments, courses, enrolments, testHasher,
                              database, events, clock);
    const int workflowDepartmentId = adminService.createDepartment(*workflowAdmin, kWorkflowDepartment);
    const int workflowHodId = adminService.createUser(*workflowAdmin, kWorkflowHodName,
        Role::HeadOfDepartment, kWorkflowPassword, workflowDepartmentId);
    const int workflowLecturerId = adminService.createUser(*workflowAdmin, kWorkflowLecturerName,
        Role::Lecturer, kWorkflowPassword, workflowDepartmentId);
    const int workflowStudentAId = adminService.createUser(*workflowAdmin, kWorkflowStudentAName,
        Role::Student, kWorkflowPassword, workflowDepartmentId);
    const int workflowStudentBId = adminService.createUser(*workflowAdmin, kWorkflowStudentBName,
        Role::Student, kWorkflowPassword, workflowDepartmentId);
    const int workflowStudentCId = adminService.createUser(*workflowAdmin, kWorkflowStudentCName,
        Role::Student, kWorkflowPassword, workflowDepartmentId);
    const int workflowCourseId = adminService.createCourse(*workflowAdmin, kWorkflowCourseCode,
        kWorkflowCourseTitle, workflowDepartmentId);
    adminService.assignLecturerToCourse(*workflowAdmin, workflowCourseId, workflowLecturerId);
    adminService.enrollStudent(*workflowAdmin, workflowCourseId, workflowStudentAId);
    adminService.enrollStudent(*workflowAdmin, workflowCourseId, workflowStudentBId);
    adminService.enrollStudent(*workflowAdmin, workflowCourseId, workflowStudentCId);

    auto lecturer = users.getById(workflowLecturerId);
    auto workflowHod = users.getById(workflowHodId);
    AssignmentService assignmentService(database, courses, assignments, events, clock);
    const int assignment = assignmentService.createAssignment(*lecturer, workflowCourseId,
        kWorkflowAssignmentTitle, kWorkflowDeadlineLocal, kMaximumMark).assignment.getId();
    ExtensionService extensionService(database, assignments, courses, enrolments,
                                      extensions, users, events, clock);
    extensionService.grantExtension(*lecturer, assignment, workflowStudentCId,
                                    kExtensionDeadline, kExtensionReason);

    FixedClock onTimeClock("2035-05-31T12:00:00Z");
    StudentService onTimeStudents(courses, assignments, submissions, versions,
                                  extensions, departments, users, onTimeClock);
    onTimeStudents.submit(workflowStudentAId, assignment, "Student A first version");
    onTimeStudents.submit(workflowStudentAId, assignment, "Student A resubmission");
    const int studentASubmission = submissions.getByAssignmentAndStudent(
        assignment, workflowStudentAId)->getId();
    ASSERT_EQ(versions.getLatestVersion(studentASubmission), 2);

    FixedClock afterDeadlineClock(kLateTime);
    StudentService afterDeadlineStudents(courses, assignments, submissions, versions,
                                         extensions, departments, users, afterDeadlineClock);
    afterDeadlineStudents.submit(workflowStudentBId, assignment, "Student B late work");
    afterDeadlineStudents.submit(workflowStudentCId, assignment, "Student C extended work");

    const int studentBSubmission = submissions.getByAssignmentAndStudent(
        assignment, workflowStudentBId)->getId();
    const int studentCSubmission = submissions.getByAssignmentAndStudent(
        assignment, workflowStudentCId)->getId();
    EXPECT_EQ(submissions.getById(studentBSubmission)->getState(), SubmissionState::Late);
    EXPECT_EQ(submissions.getById(studentCSubmission)->getState(), SubmissionState::Submitted);

    StandardLatePenalty penalty;
    MarkingService marking(database, submissions, audit, assignments, courses, versions,
        users, extensions, penalty, events, afterDeadlineClock);
    marking.markSubmission(*lecturer, studentASubmission, kStudentARaw, kFeedback,
                           MarkingService::InitialMarkReason);
    marking.markSubmission(*lecturer, studentBSubmission, kStudentBRaw, kFeedback,
                           MarkingService::InitialMarkReason);
    marking.markSubmission(*lecturer, studentCSubmission, kMaximumMark, kFeedback,
                           MarkingService::InitialMarkReason);
    EXPECT_DOUBLE_EQ(submissions.getById(studentBSubmission)->getPenaltyPercent(), 10.0);
    EXPECT_DOUBLE_EQ(submissions.getById(studentCSubmission)->getPenaltyPercent(), 0.0);

    EXPECT_FALSE(afterDeadlineStudents.listSubmissions(workflowStudentAId).back().markReleased);
    EXPECT_FALSE(afterDeadlineStudents.listSubmissions(workflowStudentBId).back().markReleased);
    ModerationService workflowModeration(database, submissions, audit, courses, assignments,
        users, events, clock);
    workflowModeration.approveSubmission(*workflowHod, studentASubmission, kModerationReason);
    workflowModeration.adjustSubmissionMark(*workflowHod, studentBSubmission,
        kStudentBAdjusted, kAdjustmentReason);
    workflowModeration.approveSubmission(*workflowHod, studentCSubmission, kModerationReason);
    EXPECT_FALSE(afterDeadlineStudents.listSubmissions(workflowStudentBId).back().markReleased);
    EXPECT_THROW(workflowModeration.releaseAssignmentMarks(*otherHod, assignment), AuthorisationException);
    workflowModeration.releaseAssignmentMarks(*workflowHod, assignment);

    const auto releasedA = afterDeadlineStudents.listSubmissions(workflowStudentAId);
    const auto releasedB = afterDeadlineStudents.listSubmissions(workflowStudentBId);
    const auto releasedC = afterDeadlineStudents.listSubmissions(workflowStudentCId);
    ASSERT_TRUE(releasedA.back().markReleased);
    ASSERT_TRUE(releasedB.back().markReleased);
    ASSERT_TRUE(releasedC.back().markReleased);
    EXPECT_DOUBLE_EQ(releasedA.back().mark, kStudentARaw);
    EXPECT_DOUBLE_EQ(releasedB.back().mark, 50.0);
    EXPECT_DOUBLE_EQ(releasedC.back().mark, kMaximumMark);
    EXPECT_EQ(releasedB.back().feedback, kFeedback);

    const auto bAudit = workflowModeration.auditForSubmission(*workflowHod, studentBSubmission);
    ASSERT_EQ(bAudit.size(), 2u);
    const auto adjustment = std::find_if(bAudit.begin(), bAudit.end(), [kAdjustmentReason](const MarkAuditEntry& entry) {
        return entry.getReason() == kAdjustmentReason;
    });
    ASSERT_NE(adjustment, bAudit.end());
    EXPECT_EQ(adjustment->getChangedBy(), workflowHodId);
    EXPECT_DOUBLE_EQ(adjustment->getOldValue(), 60.0);
    EXPECT_DOUBLE_EQ(adjustment->getNewValue(), 50.0);
}
