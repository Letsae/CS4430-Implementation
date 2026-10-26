#include "SubmissionService.h"
#include "domain/LmsException.h"
#include <cmath>

namespace Lms {

SubmissionService::SubmissionService(ISubmissionRepository& submissionRepo,
                                     IAssignmentRepository& assignmentRepo,
                                     IExtensionRepository& extensionRepo,
                                     EventPublisher& eventPublisher,
                                     const Clock& clock)
    : m_submissionRepo(submissionRepo), m_assignmentRepo(assignmentRepo),
      m_extensionRepo(extensionRepo), m_eventPublisher(eventPublisher), m_clock(clock) {}

void SubmissionService::submitAssignment(int studentId, int assignmentId, const std::string& content) {
    // Get assignment
    auto assignment = m_assignmentRepo.getById(assignmentId);

    // Check if submission already exists
    try {
        auto existingSubmission = m_submissionRepo.getByAssignmentAndStudent(assignmentId, studentId);

        // Check if resubmission is allowed
        std::string effectiveDeadline = getEffectiveDeadline(assignmentId, studentId);
        if (!canResubmit(*existingSubmission, effectiveDeadline)) {
            throw ResubmissionClosedException("Resubmission is not allowed after the effective deadline");
        }

        // Add new version
        // TODO: Implement submission version saving
        // For now, we'll just update the submission

    } catch (const NotFoundException&) {
        // First submission - create new submission record
        Submission submission(0, assignmentId, studentId, SubmissionState::NotSubmitted);
        std::string now = m_clock.now();
        submission.setFirstSubmittedAt(now);

        // Determine if late
        std::string effectiveDeadline = getEffectiveDeadline(assignmentId, studentId);
        auto deadlineTime = TimeUtils::fromIsoString(effectiveDeadline);
        auto submitTime = TimeUtils::fromIsoString(now);

        if (submitTime > deadlineTime) {
            submission.setState(SubmissionState::Late);
            // Publish late submission event
            LateSubmissionEvent event(0, studentId, assignmentId);
            m_eventPublisher.publish(event);
        } else {
            submission.setState(SubmissionState::Submitted);
        }

        m_submissionRepo.save(submission);
    }
}

std::unique_ptr<Submission> SubmissionService::getSubmission(int submissionId) {
    return m_submissionRepo.getById(submissionId);
}

std::vector<Submission> SubmissionService::getStudentSubmissions(int studentId) {
    return m_submissionRepo.getByStudent(studentId);
}

std::string SubmissionService::getEffectiveDeadline(int assignmentId, int studentId) {
    // Check for extension
    auto extension = m_extensionRepo.getByAssignmentAndStudent(assignmentId, studentId);
    if (extension) {
        return extension->getNewDeadline();
    }

    // No extension, use assignment deadline
    auto assignment = m_assignmentRepo.getById(assignmentId);
    return assignment->getDeadline();
}

bool SubmissionService::canResubmit(const Submission& submission, const std::string& effectiveDeadline) {
    std::string now = m_clock.now();
    auto nowTime = TimeUtils::fromIsoString(now);
    auto deadlineTime = TimeUtils::fromIsoString(effectiveDeadline);

    // Can resubmit only if current time is before or at effective deadline
    return nowTime <= deadlineTime;
}

} // namespace Lms
