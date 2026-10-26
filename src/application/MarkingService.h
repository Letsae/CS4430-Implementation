#pragma once

#include "domain/Entities.h"
#include "domain/User.h"
#include "data/SubmissionRepository.h"
#include "data/AssignmentRepository.h"
#include "data/ExtensionRepository.h"
#include "data/MarkAuditRepository.h"
#include "domain/LatePenaltyStrategy.h"
#include "domain/Observer.h"
#include "util/Clock.h"
#include <memory>

namespace Lms {

/**
 * Service for marking submissions and applying penalties.
 */
class MarkingService {
public:
    MarkingService(ISubmissionRepository& submissionRepo,
                   IMarkAuditRepository& auditRepo,
                   IAssignmentRepository& assignmentRepo,
                   IExtensionRepository& extensionRepo,
                   ILatePenaltyStrategy& penaltyStrategy,
                   EventPublisher& eventPublisher,
                   const Clock& clock);

    /**
     * Mark a submission with a raw mark.
     * @param submissionId The submission ID
     * @param lecturerId The lecturer ID doing the marking
     * @param rawMark The raw mark (before penalty)
     * @param feedback The feedback text
     * @throws AuthorisationException if user is not the lecturer
     */
    void markSubmission(int submissionId, int lecturerId, double rawMark, const std::string& feedback);

    /**
     * Apply late penalty to a submission.
     * @param submissionId The submission ID
     * @param changedBy The user ID applying the penalty
     * @param reason The reason for penalty application
     */
    void applyLatePenalty(int submissionId, int changedBy, const std::string& reason);

    /**
     * Calculate the final mark with penalty.
     */
    double calculateFinalMark(const Submission& submission, const Assignment& assignment);

private:
    ISubmissionRepository& m_submissionRepo;
    IMarkAuditRepository& m_auditRepo;
    IAssignmentRepository& m_assignmentRepo;
    IExtensionRepository& m_extensionRepo;
    ILatePenaltyStrategy& m_penaltyStrategy;
    EventPublisher& m_eventPublisher;
    const Clock& m_clock;

    /**
     * Write an audit entry for a mark change.
     */
    void writeAuditEntry(int submissionId, int changedBy, double oldValue, double newValue, const std::string& reason);

    /**
     * Round a mark to 2 decimal places, half up.
     */
    double roundMark(double mark);
};

} // namespace Lms
