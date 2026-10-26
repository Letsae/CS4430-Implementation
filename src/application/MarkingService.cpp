#include "MarkingService.h"
#include "domain/SubmissionStateMachine.h"
#include "domain/LmsException.h"
#include <cmath>
#include <sstream>

namespace Lms {

MarkingService::MarkingService(ISubmissionRepository& submissionRepo,
                                 IMarkAuditRepository& auditRepo,
                                 IAssignmentRepository& assignmentRepo,
                                 IExtensionRepository& extensionRepo,
                                 ILatePenaltyStrategy& penaltyStrategy,
                                 EventPublisher& eventPublisher,
                                 const Clock& clock)
    : m_submissionRepo(submissionRepo), m_auditRepo(auditRepo),
      m_assignmentRepo(assignmentRepo), m_extensionRepo(extensionRepo),
      m_penaltyStrategy(penaltyStrategy), m_eventPublisher(eventPublisher), m_clock(clock) {}

void MarkingService::markSubmission(int submissionId, int lecturerId, double rawMark, const std::string& feedback) {
    auto submission = m_submissionRepo.getById(submissionId);
    auto assignment = m_assignmentRepo.getById(submission->getAssignmentId());

    // Validate mark
    if (rawMark < 0 || rawMark > assignment->getMaxMark()) {
        throw ValidationException("Mark must be between 0 and " +
                                  std::to_string(assignment->getMaxMark()));
    }

    // Transition state to Marked
    SubmissionStateMachine::transition(*submission, SubmissionState::Marked);

    // Set raw mark and feedback
    double oldMark = submission->getRawMark();
    submission->setRawMark(rawMark);
    submission->setFeedback(feedback);

    // Calculate penalty
    double penaltyPercent = m_penaltyStrategy.calculatePenalty(*submission, *assignment, m_clock);
    submission->setPenaltyPercent(penaltyPercent);

    // Calculate final mark
    double finalMark = calculateFinalMark(*submission, *assignment);
    submission->setFinalMark(finalMark);

    // Save submission
    // Note: In a real implementation, we'd need an update method in the repository
    // For now, we'll just write the audit entry
    m_submissionRepo.save(*submission);

    // Write audit entry
    std::ostringstream reason;
    reason << "Initial marking by lecturer " << lecturerId;
    writeAuditEntry(submissionId, lecturerId, oldMark, finalMark, reason.str());

    // Publish mark changed event
    MarkChangedEvent event(submissionId, lecturerId, oldMark, finalMark);
    m_eventPublisher.publish(event);
}

void MarkingService::applyLatePenalty(int submissionId, int changedBy, const std::string& reason) {
    auto submission = m_submissionRepo.getById(submissionId);
    auto assignment = m_assignmentRepo.getById(submission->getAssignmentId());

    double oldFinalMark = submission->getFinalMark();

    // Calculate penalty
    double penaltyPercent = m_penaltyStrategy.calculatePenalty(*submission, *assignment, m_clock);
    submission->setPenaltyPercent(penaltyPercent);

    // Recalculate final mark
    double finalMark = calculateFinalMark(*submission, *assignment);
    submission->setFinalMark(finalMark);

    // Save
    m_submissionRepo.save(*submission);

    // Write audit entry
    writeAuditEntry(submissionId, changedBy, oldFinalMark, finalMark, reason);

    // Publish event
    MarkChangedEvent event(submissionId, changedBy, oldFinalMark, finalMark);
    m_eventPublisher.publish(event);
}

double MarkingService::calculateFinalMark(const Submission& submission, const Assignment& assignment) {
    double rawMark = submission.getRawMark();
    double penaltyPercent = submission.getPenaltyPercent();

    // Apply penalty
    double penaltyAmount = (assignment.getMaxMark() * penaltyPercent) / 100.0;
    double finalMark = rawMark - penaltyAmount;

    // Clamp to [0, maxMark]
    finalMark = std::max(0.0, std::min(finalMark, assignment.getMaxMark()));

    // Round to 2 decimal places, half up
    return roundMark(finalMark);
}

void MarkingService::writeAuditEntry(int submissionId, int changedBy, double oldValue, double newValue, const std::string& reason) {
    if (reason.empty()) {
        throw ValidationException("Audit reason cannot be empty");
    }

    std::string now = m_clock.now();
    MarkAuditEntry entry(0, submissionId, changedBy, now, oldValue, newValue, reason);
    m_auditRepo.add(entry);
}

double MarkingService::roundMark(double mark) {
    // Round to 2 decimal places, half up
    return std::round(mark * 100.0) / 100.0;
}

} // namespace Lms
