#pragma once

#include "domain/Entities.h"
#include "domain/User.h"
#include "data/SubmissionRepository.h"
#include "data/MarkAuditRepository.h"
#include "domain/Observer.h"
#include "util/Clock.h"
#include <memory>

namespace Lms {

/**
 * Service for HoD moderation of marks.
 */
class ModerationService {
public:
    ModerationService(ISubmissionRepository& submissionRepo,
                      IMarkAuditRepository& auditRepo,
                      EventPublisher& eventPublisher,
                      const Clock& clock);

    /**
     * Moderate a submission (approve or return to lecturer).
     * @param submissionId The submission ID
     * @param hodId The HoD ID doing the moderation
     * @param approved Whether to approve (true) or return (false)
     * @param comment The moderation comment
     * @throws AuthorisationException if user is not HoD
     */
    void moderateSubmission(int submissionId, int hodId, bool approved, const std::string& comment);

    /**
     * Release marks for all marked submissions in a department.
     * @param departmentId The department ID
     * @param hodId The HoD ID releasing the marks
     */
    void releaseMarks(int departmentId, int hodId);

    /**
     * Get all marked submissions for a department.
     */
    std::vector<Submission> getMarkedSubmissionsForDepartment(int departmentId);

private:
    ISubmissionRepository& m_submissionRepo;
    IMarkAuditRepository& m_auditRepo;
    EventPublisher& m_eventPublisher;
    const Clock& m_clock;

    /**
     * Write an audit entry for a mark change during moderation.
     */
    void writeAuditEntry(int submissionId, int changedBy, double oldValue, double newValue, const std::string& reason);
};

} // namespace Lms
