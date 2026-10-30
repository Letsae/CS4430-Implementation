#pragma once

#include "domain/Entities.h"
#include "domain/User.h"
#include "data/SubmissionRepository.h"
#include "data/AssignmentRepository.h"
#include "data/ExtensionRepository.h"
#include "data/MarkAuditRepository.h"
#include "data/CourseRepository.h"
#include "data/Database.h"
#include "data/SubmissionVersionRepository.h"
#include "data/UserRepository.h"
#include "domain/LatePenaltyStrategy.h"
#include "domain/Observer.h"
#include "util/Clock.h"
#include <memory>

namespace Lms {

struct LecturerSubmissionInfo {
    Submission submission;
    std::string student;
    std::string courseCode;
    std::string assignmentTitle;
    std::string latestVersionSubmittedAt;
};

/**
 * Service for marking submissions and applying penalties.
 */
class MarkingService {
public:
    MarkingService(Database& database,
                   ISubmissionRepository& submissionRepo,
                   IMarkAuditRepository& auditRepo,
                   IAssignmentRepository& assignmentRepo,
                   ICourseRepository& courseRepo,
                   ISubmissionVersionRepository& versionRepo,
                   IUserRepository& userRepo,
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
    void markSubmission(const User& lecturer, int submissionId, double rawMark,
                        const std::string& feedback, const std::string& reason);
    std::vector<Assignment> listAssignments(const User& lecturer) const;
    std::vector<LecturerSubmissionInfo> listSubmissions(const User& lecturer, int assignmentId) const;
    static constexpr const char* InitialMarkReason = "INITIAL_MARKING";

    /**
     * Calculate the final mark with penalty.
     */
    double calculateFinalMark(const Submission& submission, const Assignment& assignment);

private:
    Database& m_database;
    ISubmissionRepository& m_submissionRepo;
    IMarkAuditRepository& m_auditRepo;
    IAssignmentRepository& m_assignmentRepo;
    ICourseRepository& m_courseRepo;
    ISubmissionVersionRepository& m_versionRepo;
    IUserRepository& m_userRepo;
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
    void requireLecturerForAssignment(const User& lecturer, int assignmentId) const;
    static void requireFeedback(const std::string& feedback);
    static void requireReason(const std::string& reason);
};

} // namespace Lms
