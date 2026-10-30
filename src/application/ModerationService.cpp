#include "ModerationService.h"
#include "domain/LmsException.h"
#include "domain/SubmissionStateMachine.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace Lms {

ModerationService::ModerationService(Database& database, ISubmissionRepository& submissionRepo,
        IMarkAuditRepository& auditRepo, ICourseRepository& courseRepo,
        IAssignmentRepository& assignmentRepo, IUserRepository& userRepo,
        EventPublisher& eventPublisher, const Clock& clock)
    : m_database(database), m_submissionRepo(submissionRepo), m_auditRepo(auditRepo),
      m_courseRepo(courseRepo), m_assignmentRepo(assignmentRepo), m_userRepo(userRepo),
      m_eventPublisher(eventPublisher), m_clock(clock) {}

int ModerationService::requireDepartment(const User& hod) const {
    if (hod.getRole() != Role::HeadOfDepartment || !hod.canPerform(Action::ModerateSubmission))
        throw AuthorisationException("Only a Head of Department can moderate marks");
    const auto* departmentHead = dynamic_cast<const HeadOfDepartment*>(&hod);
    if (!departmentHead || departmentHead->getDepartmentId() <= 0)
        throw AuthorisationException("Head of Department has no assigned department");
    return departmentHead->getDepartmentId();
}

std::unique_ptr<Assignment> ModerationService::requireScopedAssignment(const User& hod, int assignmentId) const {
    const int departmentId = requireDepartment(hod);
    auto assignment = m_assignmentRepo.getById(assignmentId);
    auto course = m_courseRepo.getById(assignment->getCourseId());
    if (course->getDepartmentId() != departmentId)
        throw AuthorisationException("Assignment is outside your department");
    return assignment;
}

std::unique_ptr<Submission> ModerationService::requireScopedSubmission(const User& hod, int submissionId) const {
    const int departmentId = requireDepartment(hod);
    auto submission = m_submissionRepo.getById(submissionId);
    auto assignment = m_assignmentRepo.getById(submission->getAssignmentId());
    auto course = m_courseRepo.getById(assignment->getCourseId());
    if (course->getDepartmentId() != departmentId)
        throw AuthorisationException("Submission is outside your department");
    return submission;
}

std::vector<Course> ModerationService::listDepartmentCourses(const User& hod) const {
    return m_courseRepo.getByDepartment(requireDepartment(hod));
}

std::vector<Assignment> ModerationService::listDepartmentAssignments(const User& hod) const {
    std::vector<Assignment> result;
    for (const auto& course : listDepartmentCourses(hod)) {
        auto assignments = m_assignmentRepo.getByCourse(course.getId());
        result.insert(result.end(), assignments.begin(), assignments.end());
    }
    return result;
}

std::vector<ModerationReview> ModerationService::reviewMarkedSubmissions(const User& hod) const {
    const int departmentId = requireDepartment(hod);
    std::vector<ModerationReview> result;
    for (const auto& course : m_courseRepo.getByDepartment(departmentId)) {
        for (const auto& assignment : m_assignmentRepo.getByCourse(course.getId())) {
            for (const auto& submission : m_submissionRepo.getByAssignment(assignment.getId())) {
                if (submission.getState() != SubmissionState::Marked) continue;
                auto student = m_userRepo.getById(submission.getStudentId());
                auto lecturer = m_userRepo.getById(assignment.getCreatedBy());
                result.push_back({submission, student->getUsername(), course.getCode(),
                                  assignment.getTitle(), lecturer->getUsername()});
            }
        }
    }
    return result;
}

void ModerationService::requireReason(const std::string& reason) const {
    if (reason.empty() || std::all_of(reason.begin(), reason.end(), [](unsigned char c) { return std::isspace(c); }))
        throw ValidationException("A non-blank moderation reason is required");
}

void ModerationService::approveSubmission(const User& hod, int submissionId, const std::string& reason) {
    requireReason(reason);
    auto submission = requireScopedSubmission(hod, submissionId);
    if (submission->getState() != SubmissionState::Marked)
        throw InvalidTransitionException("Only marked submissions can be moderated");
    const double currentMark = submission->getFinalMark();
    SubmissionStateMachine::transition(*submission, SubmissionState::Moderated);
    TransactionGuard transaction(m_database);
    m_submissionRepo.save(*submission);
    appendAudit(submissionId, hod.getId(), currentMark, currentMark, "APPROVED: " + reason);
    transaction.commit();
}

void ModerationService::adjustSubmissionMark(const User& hod, int submissionId,
        double rawMark, const std::string& reason) {
    requireReason(reason);
    auto submission = requireScopedSubmission(hod, submissionId);
    if (submission->getState() != SubmissionState::Marked)
        throw InvalidTransitionException("Only marked submissions can be moderated");
    auto assignment = m_assignmentRepo.getById(submission->getAssignmentId());
    if (!std::isfinite(rawMark) || rawMark < 0.0 || rawMark > assignment->getMaxMark())
        throw ValidationException("Mark must be between 0 and the assignment maximum");

    const double oldRawMark = submission->getRawMark();
    const double oldValue = submission->getFinalMark();
    const double penalty = assignment->getMaxMark() * submission->getPenaltyPercent() / 100.0;
    const double newValue = std::round(std::max(0.0, rawMark - penalty) * 100.0) / 100.0;
    submission->setRawMark(rawMark);
    submission->setFinalMark(newValue);
    SubmissionStateMachine::transition(*submission, SubmissionState::Moderated);

    TransactionGuard transaction(m_database);
    m_submissionRepo.save(*submission);
    if (oldRawMark != rawMark || oldValue != newValue)
        appendAudit(submissionId, hod.getId(), oldValue, newValue, reason);
    MarkChangedEvent event(submissionId, hod.getId(), oldValue, newValue);
    m_eventPublisher.publish(event);
    transaction.commit();
}

void ModerationService::releaseAssignmentMarks(const User& hod, int assignmentId) {
    auto assignment = requireScopedAssignment(hod, assignmentId);
    auto course = m_courseRepo.getById(assignment->getCourseId());
    std::vector<Submission> moderated;
    for (const auto& submission : m_submissionRepo.getByAssignment(assignmentId))
        if (submission.getState() == SubmissionState::Moderated) moderated.push_back(submission);
    if (moderated.empty()) throw ValidationException("No moderated submissions are available to release");

    TransactionGuard transaction(m_database);
    std::vector<int> studentIds;
    for (auto& submission : moderated) {
        SubmissionStateMachine::transition(submission, SubmissionState::Released);
        m_submissionRepo.save(submission);
        studentIds.push_back(submission.getStudentId());
    }
    m_eventPublisher.publish(MarksReleasedEvent(course->getId(), course->getDepartmentId(), std::move(studentIds)));
    transaction.commit();
}

std::vector<MarkAuditEntry> ModerationService::auditForSubmission(const User& hod, int submissionId) const {
    if (!hod.canPerform(Action::ViewMarkAudit)) throw AuthorisationException("User cannot view mark audit entries");
    requireScopedSubmission(hod, submissionId);
    return m_auditRepo.getBySubmission(submissionId);
}

std::vector<MarkAuditEntry> ModerationService::auditForAssignment(const User& hod, int assignmentId) const {
    if (!hod.canPerform(Action::ViewMarkAudit)) throw AuthorisationException("User cannot view mark audit entries");
    requireScopedAssignment(hod, assignmentId);
    std::vector<MarkAuditEntry> result;
    for (const auto& submission : m_submissionRepo.getByAssignment(assignmentId)) {
        auto entries = m_auditRepo.getBySubmission(submission.getId());
        result.insert(result.end(), entries.begin(), entries.end());
    }
    std::sort(result.begin(), result.end(), [](const MarkAuditEntry& lhs, const MarkAuditEntry& rhs) {
        if (lhs.getChangedAt() != rhs.getChangedAt()) return lhs.getChangedAt() < rhs.getChangedAt();
        return lhs.getId() < rhs.getId();
    });
    return result;
}

void ModerationService::appendAudit(int submissionId, int userId, double oldValue,
        double newValue, const std::string& reason) {
    m_auditRepo.add(MarkAuditEntry(0, submissionId, userId, m_clock.now(), oldValue, newValue, reason));
}

} // namespace Lms
