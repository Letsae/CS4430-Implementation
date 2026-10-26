#pragma once

#include "domain/Entities.h"
#include "domain/User.h"
#include "data/SubmissionRepository.h"
#include "data/AssignmentRepository.h"
#include "data/ExtensionRepository.h"
#include "domain/Observer.h"
#include "util/Clock.h"
#include <memory>

namespace Lms {

// Forward declaration
class IExtensionRepository;

/**
 * Service for handling assignment submissions.
 */
class SubmissionService {
public:
    SubmissionService(ISubmissionRepository& submissionRepo,
                      IAssignmentRepository& assignmentRepo,
                      IExtensionRepository& extensionRepo,
                      EventPublisher& eventPublisher,
                      const Clock& clock);

    /**
     * Submit an assignment for a student.
     * @param studentId The student ID
     * @param assignmentId The assignment ID
     * @param content The submission content (file path or data)
     * @throws AuthorisationException if user is not the student
     * @throws ResubmissionClosedException if resubmission is not allowed
     */
    void submitAssignment(int studentId, int assignmentId, const std::string& content);

    /**
     * Get a submission by ID.
     */
    std::unique_ptr<Submission> getSubmission(int submissionId);

    /**
     * Get all submissions for a student.
     */
    std::vector<Submission> getStudentSubmissions(int studentId);

    /**
     * Get the effective deadline for a student (considering extensions).
     */
    std::string getEffectiveDeadline(int assignmentId, int studentId);

private:
    ISubmissionRepository& m_submissionRepo;
    IAssignmentRepository& m_assignmentRepo;
    IExtensionRepository& m_extensionRepo;
    EventPublisher& m_eventPublisher;
    const Clock& m_clock;

    /**
     * Check if resubmission is allowed.
     */
    bool canResubmit(const Submission& submission, const std::string& effectiveDeadline);
};

} // namespace Lms
