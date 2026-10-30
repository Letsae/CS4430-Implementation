#include "MarkingService.h"
#include "domain/SubmissionStateMachine.h"
#include "domain/LmsException.h"
#include <cmath>
#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>

namespace Lms {

MarkingService::MarkingService(Database& database,
                                 ISubmissionRepository& submissionRepo,
                                 IMarkAuditRepository& auditRepo,
                                 IAssignmentRepository& assignmentRepo,
                                 ICourseRepository& courseRepo,
                                 ISubmissionVersionRepository& versionRepo,
                                 IUserRepository& userRepo,
                                 IExtensionRepository& extensionRepo,
                                 ILatePenaltyStrategy& penaltyStrategy,
                                 EventPublisher& eventPublisher,
                                 const Clock& clock)
    : m_database(database), m_submissionRepo(submissionRepo), m_auditRepo(auditRepo),
      m_assignmentRepo(assignmentRepo), m_courseRepo(courseRepo), m_versionRepo(versionRepo),
      m_userRepo(userRepo), m_extensionRepo(extensionRepo),
      m_penaltyStrategy(penaltyStrategy), m_eventPublisher(eventPublisher), m_clock(clock) {}

void MarkingService::requireLecturerForAssignment(const User& lecturer, int assignmentId) const {
    if (lecturer.getRole() != Role::Lecturer || !lecturer.canPerform(Action::MarkSubmission))
        throw AuthorisationException("Only lecturers can mark submissions");
    if (!m_userRepo.isActive(lecturer.getId())) throw AuthorisationException("Lecturer account is inactive");
    auto assignment = m_assignmentRepo.getById(assignmentId);
    const auto courses = m_courseRepo.getByLecturer(lecturer.getId());
    if (std::none_of(courses.begin(), courses.end(), [&assignment](const Course& course) {
            return course.getId() == assignment->getCourseId();
        }))
        throw AuthorisationException("Submission is outside your assigned courses");
}

void MarkingService::requireFeedback(const std::string& feedback) {
    if (feedback.empty() || std::all_of(feedback.begin(), feedback.end(), [](unsigned char c) { return std::isspace(c); }))
        throw ValidationException("Written feedback is required");
}

void MarkingService::requireReason(const std::string& reason) {
    if (reason.empty() || std::all_of(reason.begin(), reason.end(), [](unsigned char c) { return std::isspace(c); }))
        throw ValidationException("A non-blank audit reason is required");
}

std::vector<Assignment> MarkingService::listAssignments(const User& lecturer) const {
    if (lecturer.getRole() != Role::Lecturer || !lecturer.canPerform(Action::ViewSubmission))
        throw AuthorisationException("Only lecturers can view their submissions");
    if (!m_userRepo.isActive(lecturer.getId())) throw AuthorisationException("Lecturer account is inactive");
    std::vector<Assignment> result;
    for (const auto& course : m_courseRepo.getByLecturer(lecturer.getId())) {
        auto assignments = m_assignmentRepo.getByCourse(course.getId());
        result.insert(result.end(), assignments.begin(), assignments.end());
    }
    return result;
}

std::vector<LecturerSubmissionInfo> MarkingService::listSubmissions(const User& lecturer, int assignmentId) const {
    requireLecturerForAssignment(lecturer, assignmentId);
    auto assignment = m_assignmentRepo.getById(assignmentId);
    auto course = m_courseRepo.getById(assignment->getCourseId());
    std::vector<LecturerSubmissionInfo> result;
    for (const auto& submission : m_submissionRepo.getByAssignment(assignmentId)) {
        auto student = m_userRepo.getById(submission.getStudentId());
        const int latest = m_versionRepo.getLatestVersion(submission.getId());
        std::string latestTime;
        if (latest > 0) latestTime = m_versionRepo.get(submission.getId(), latest)->getSubmittedAt();
        result.push_back({submission, student->getUsername(), course->getCode(), assignment->getTitle(), latestTime});
    }
    return result;
}

void MarkingService::markSubmission(const User& lecturer, int submissionId, double rawMark,
        const std::string& feedback, const std::string& reason) {
    auto submission = m_submissionRepo.getById(submissionId);
    auto assignment = m_assignmentRepo.getById(submission->getAssignmentId());
    requireLecturerForAssignment(lecturer, assignment->getId());
    requireFeedback(feedback);
    requireReason(reason);

    // Validate mark
    if (!std::isfinite(rawMark) || rawMark < 0 || rawMark > assignment->getMaxMark()) {
        throw ValidationException("Mark must be between 0 and " +
                                  std::to_string(assignment->getMaxMark()));
    }

    const auto currentState = submission->getState();
    if (currentState != SubmissionState::Submitted && currentState != SubmissionState::Late &&
        currentState != SubmissionState::Marked)
        throw InvalidTransitionException("Only submitted or previously marked work can be marked");
    if (currentState != SubmissionState::Marked) SubmissionStateMachine::transition(*submission, SubmissionState::Marked);

    // Set raw mark and feedback
    double oldMark = submission->getFinalMark();
    submission->setRawMark(rawMark);
    submission->setFeedback(feedback);

    // Calculate penalty
    auto effectiveAssignment = *assignment;
    auto extension = m_extensionRepo.getByAssignmentAndStudent(assignment->getId(), submission->getStudentId());
    if (extension) effectiveAssignment.setDeadline(extension->getNewDeadline());
    double penaltyPercent = m_penaltyStrategy.calculatePenalty(*submission, effectiveAssignment, m_clock);
    submission->setPenaltyPercent(penaltyPercent);

    // Calculate final mark
    double finalMark = calculateFinalMark(*submission, *assignment);
    submission->setFinalMark(finalMark);

    TransactionGuard transaction(m_database);
    m_submissionRepo.save(*submission);

    writeAuditEntry(submissionId, lecturer.getId(), oldMark, finalMark, reason);

    MarkChangedEvent event(submissionId, lecturer.getId(), oldMark, finalMark);
    m_eventPublisher.publish(event);
    transaction.commit();
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
