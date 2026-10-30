#pragma once

#include "data/AssignmentRepository.h"
#include "data/CourseRepository.h"
#include "data/Database.h"
#include "data/MarkAuditRepository.h"
#include "data/SubmissionRepository.h"
#include "data/UserRepository.h"
#include "domain/Entities.h"
#include "domain/Observer.h"
#include "util/Clock.h"

namespace Lms {

struct ModerationReview {
    Submission submission;
    std::string student;
    std::string courseCode;
    std::string assignmentTitle;
    std::string lecturer;
};

class ModerationService {
public:
    ModerationService(Database& database, ISubmissionRepository& submissionRepo,
                      IMarkAuditRepository& auditRepo, ICourseRepository& courseRepo,
                      IAssignmentRepository& assignmentRepo, IUserRepository& userRepo,
                      EventPublisher& eventPublisher, const Clock& clock);

    std::vector<Course> listDepartmentCourses(const User& hod) const;
    std::vector<Assignment> listDepartmentAssignments(const User& hod) const;
    std::vector<ModerationReview> reviewMarkedSubmissions(const User& hod) const;
    void approveSubmission(const User& hod, int submissionId, const std::string& reason);
    void adjustSubmissionMark(const User& hod, int submissionId, double rawMark, const std::string& reason);
    void releaseAssignmentMarks(const User& hod, int assignmentId);
    std::vector<MarkAuditEntry> auditForSubmission(const User& hod, int submissionId) const;
    std::vector<MarkAuditEntry> auditForAssignment(const User& hod, int assignmentId) const;

private:
    Database& m_database;
    ISubmissionRepository& m_submissionRepo;
    IMarkAuditRepository& m_auditRepo;
    ICourseRepository& m_courseRepo;
    IAssignmentRepository& m_assignmentRepo;
    IUserRepository& m_userRepo;
    EventPublisher& m_eventPublisher;
    const Clock& m_clock;

    int requireDepartment(const User& hod) const;
    std::unique_ptr<Assignment> requireScopedAssignment(const User& hod, int assignmentId) const;
    std::unique_ptr<Submission> requireScopedSubmission(const User& hod, int submissionId) const;
    void requireReason(const std::string& reason) const;
    void appendAudit(int submissionId, int userId, double oldValue, double newValue,
                     const std::string& reason);
};

} // namespace Lms
