#include "ModerationService.h"
#include "domain/SubmissionStateMachine.h"
#include "domain/LmsException.h"
#include <sstream>

namespace Lms {

ModerationService::ModerationService(ISubmissionRepository& submissionRepo,
                                     IMarkAuditRepository& auditRepo,
                                     EventPublisher& eventPublisher,
                                     const Clock& clock)
    : m_submissionRepo(submissionRepo), m_auditRepo(auditRepo),
      m_eventPublisher(eventPublisher), m_clock(clock) {}

void ModerationService::moderateSubmission(int submissionId, int hodId, bool approved, const std::string& comment) {
    auto submission = m_submissionRepo.getById(submissionId);

    if (submission->getState() != SubmissionState::Marked) {
        throw InvalidTransitionException("Can only moderate marked submissions");
    }

    if (approved) {
        // Approve and move to Released
        SubmissionStateMachine::transition(*submission, SubmissionState::Released);
    } else {
        // Return to lecturer - move back to Marked with comment
        // For now, we'll keep it in Marked state but add comment to feedback
        std::string currentFeedback = submission->getFeedback();
        std::string newFeedback = currentFeedback + "\n[HoD Comment: " + comment + "]";
        submission->setFeedback(newFeedback);
    }

    // Save
    m_submissionRepo.save(*submission);

    // Write audit entry
    std::ostringstream reason;
    reason << "Moderation by HoD " << hodId << ": " << (approved ? "Approved" : "Returned");
    writeAuditEntry(submissionId, hodId, submission->getFinalMark(), submission->getFinalMark(), reason.str());
}

void ModerationService::releaseMarks(int departmentId, int hodId) {
    // Get all marked submissions for the department
    auto markedSubmissions = getMarkedSubmissionsForDepartment(departmentId);

    // Release all marked submissions in a transaction
    // Note: In a real implementation, we'd use a transaction here
    for (auto& submission : markedSubmissions) {
        try {
            SubmissionStateMachine::transition(submission, SubmissionState::Released);
            m_submissionRepo.save(submission);

            std::ostringstream reason;
            reason << "Bulk release by HoD " << hodId;
            writeAuditEntry(submission.getId(), hodId, submission.getFinalMark(), submission.getFinalMark(), reason.str());
        } catch (const InvalidTransitionException&) {
            // Skip submissions that can't be released
            continue;
        }
    }

    // Publish marks released event
    MarksReleasedEvent event(0, departmentId);
    m_eventPublisher.publish(event);
}

std::vector<Submission> ModerationService::getMarkedSubmissionsForDepartment(int departmentId) {
    (void)departmentId; // TODO: Implement department filtering
    // Get all marked submissions
    auto allMarked = m_submissionRepo.getByState(SubmissionState::Marked);

    // Filter by department (this would require joining with course and department tables)
    // For now, we'll return all marked submissions
    // In a real implementation, we'd add a method to the repository to filter by department
    return allMarked;
}

void ModerationService::writeAuditEntry(int submissionId, int changedBy, double oldValue, double newValue, const std::string& reason) {
    if (reason.empty()) {
        throw ValidationException("Audit reason cannot be empty");
    }

    std::string now = m_clock.now();
    MarkAuditEntry entry(0, submissionId, changedBy, now, oldValue, newValue, reason);
    m_auditRepo.add(entry);
}

} // namespace Lms
