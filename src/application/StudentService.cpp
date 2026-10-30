#include "StudentService.h"
#include "domain/LmsException.h"
#include "domain/LatePenaltyStrategy.h"
#include "domain/Observer.h"
#include "data/Database.h"
#include <cmath>
#include <chrono>
#include <memory>

namespace Lms {

StudentService::StudentService(ICourseRepository& courseRepo,
                               IAssignmentRepository& assignmentRepo,
                               ISubmissionRepository& submissionRepo,
                               ISubmissionVersionRepository& versionRepo,
                               IExtensionRepository& extensionRepo,
                               IDepartmentRepository& departmentRepo,
                               IUserRepository& userRepo,
                               const Clock& clock)
    : m_courseRepo(courseRepo), m_assignmentRepo(assignmentRepo),
      m_submissionRepo(submissionRepo), m_versionRepo(versionRepo),
      m_extensionRepo(extensionRepo), m_departmentRepo(departmentRepo),
      m_userRepo(userRepo), m_clock(clock) {}

StudentService::StudentService(Database& database, ICourseRepository& courseRepo,
        IAssignmentRepository& assignmentRepo, ISubmissionRepository& submissionRepo,
        ISubmissionVersionRepository& versionRepo, IExtensionRepository& extensionRepo,
        IDepartmentRepository& departmentRepo, IUserRepository& userRepo,
        ISettingsRepository& settings, ILatePenaltyStrategy& penaltyStrategy,
        EventPublisher& events, const Clock& clock)
    : StudentService(courseRepo, assignmentRepo, submissionRepo, versionRepo, extensionRepo,
                     departmentRepo, userRepo, clock) {
    m_database = &database;
    m_settings = &settings;
    m_penaltyStrategy = &penaltyStrategy;
    m_events = &events;
}

void StudentService::requireStudent(int studentId) const {
    if (!m_userRepo.exists(studentId)) throw NotFoundException("Student not found with ID: " + std::to_string(studentId));
    auto student = m_userRepo.getById(studentId);
    if (student->getRole() != Role::Student || !student->canPerform(Action::ViewOwnCourses))
        throw AuthorisationException("Only students can access student records");
    if (!m_userRepo.isActive(studentId)) throw AuthorisationException("Student account is inactive");
}

size_t StudentService::maxSubmissionLength() const {
    if (!m_settings) return 20000;
    const std::string configured = m_settings->getWithDefault("max_submission_length", "20000");
    try {
        size_t consumed = 0;
        const auto value = std::stoull(configured, &consumed);
        if (consumed == configured.size() && value > 0) return static_cast<size_t>(value);
    } catch (const std::exception&) {}
    throw ValidationException("Configured maximum submission length is invalid");
}

std::vector<CourseInfo> StudentService::listCourses(int studentId) {
    requireStudent(studentId);
    // Get courses for student
    auto courses = m_courseRepo.getByStudent(studentId);

    if (courses.empty()) {
        return {};
    }

    std::vector<CourseInfo> courseInfos;

    for (const auto& course : courses) {
        CourseInfo info;
        info.id = course.getId();
        info.code = course.getCode();
        info.title = course.getTitle();

        // Get department name
        try {
            auto dept = m_departmentRepo.getById(course.getDepartmentId());
            info.department = dept->getName();
        } catch (const NotFoundException&) {
            info.department = "Unknown";
        }

        info.lecturers = m_courseRepo.getLecturerUsernames(course.getId());

        auto assignments = m_assignmentRepo.getByCourse(course.getId());

        for (const auto& assignment : assignments) {
            AssignmentInfo assignInfo;
            assignInfo.id = assignment.getId();
            assignInfo.title = assignment.getTitle();
            assignInfo.deadline = assignment.getDeadline();
            assignInfo.effectiveDeadline = getEffectiveDeadline(assignment.getId(), studentId);
            assignInfo.hasExtension = (assignInfo.effectiveDeadline != assignment.getDeadline());
            assignInfo.maxMark = assignment.getMaxMark();

            try {
                auto submission = m_submissionRepo.getByAssignmentAndStudent(assignment.getId(), studentId);
                if (submission->getStudentId() != studentId) {
                    throw AuthorisationException("Cannot view another student's submission");
                }
                assignInfo.state = stateToString(submission->getState());
                assignInfo.versionCount = m_versionRepo.getLatestVersion(submission->getId());
            } catch (const NotFoundException&) {
                assignInfo.state = "NotSubmitted";
                assignInfo.versionCount = 0;
            }

            info.assignments.push_back(assignInfo);
        }

        courseInfos.push_back(info);
    }

    return courseInfos;
}

std::vector<SubmissionInfo> StudentService::listSubmissions(int studentId) {
    requireStudent(studentId);
    auto submissions = m_submissionRepo.getByStudent(studentId);

    std::vector<SubmissionInfo> submissionInfos;

    for (const auto& submission : submissions) {
        SubmissionInfo info;
        info.id = submission.getId();
        info.state = stateToString(submission.getState());
        info.firstSubmittedAt = submission.getFirstSubmittedAt();

        // Get assignment info
        try {
            auto assignment = m_assignmentRepo.getById(submission.getAssignmentId());
            info.assignmentTitle = assignment->getTitle();

            // Get course info
            auto course = m_courseRepo.getById(assignment->getCourseId());
            info.courseCode = course->getCode();
            info.courseTitle = course->getTitle();

            // Calculate days late if state is Late
            if (submission.getState() == SubmissionState::Late) {
                std::string effectiveDeadline = getEffectiveDeadline(assignment->getId(), studentId);
                info.daysLate = calculateDaysLate(submission.getFirstSubmittedAt(), effectiveDeadline);
            } else {
                info.daysLate = 0;
            }

            info.latestVersion = m_versionRepo.getLatestVersion(submission.getId());
            if (info.latestVersion > 0)
                info.latestVersionSubmittedAt = m_versionRepo.get(submission.getId(), info.latestVersion)->getSubmittedAt();

            // Mark and feedback only if Released
            if (submission.getState() == SubmissionState::Released) {
                info.markReleased = true;
                info.mark = submission.getFinalMark();
                info.feedback = submission.getFeedback();
            } else {
                info.markReleased = false;
                info.mark = 0.0;
                info.feedback = "Mark not yet released";
            }
        } catch (const NotFoundException&) {
            // Skip if assignment or course not found
            continue;
        }

        submissionInfos.push_back(info);
    }

    return submissionInfos;
}

void StudentService::submit(int studentId, int assignmentId, const std::string& content) {
    requireStudent(studentId);
    // Validate content
    if (content.empty()) {
        throw ValidationException("Submission content cannot be empty");
    }
    const size_t maxLength = maxSubmissionLength();
    if (content.length() > maxLength) {
        throw ValidationException("Submission content exceeds the configured maximum length of " +
                                  std::to_string(maxLength) + " characters");
    }

    // Get assignment
    auto assignment = m_assignmentRepo.getById(assignmentId);

    // Get course to check enrolment
    auto course = m_courseRepo.getById(assignment->getCourseId());

    // Check if student is enrolled
    auto studentCourses = m_courseRepo.getByStudent(studentId);
    bool enrolled = false;
    for (const auto& c : studentCourses) {
        if (c.getId() == course->getId()) {
            enrolled = true;
            break;
        }
    }

    if (!enrolled) {
        throw AuthorisationException("You are not enrolled in this course");
    }

    // Get effective deadline
    std::string effectiveDeadline = getEffectiveDeadline(assignmentId, studentId);
    std::string now = m_clock.now();

    auto deadlineTime = TimeUtils::fromIsoString(effectiveDeadline);
    auto submitTime = TimeUtils::fromIsoString(now);

    // Check for existing submission
    int submissionId = 0;
    int nextVersion = 1;
    bool hasExisting = false;
    try {
        auto existingSubmission = m_submissionRepo.getByAssignmentAndStudent(assignmentId, studentId);
        hasExisting = true;
        submissionId = existingSubmission->getId();
        nextVersion = m_versionRepo.getLatestVersion(submissionId) + 1;

        // Check if resubmission is allowed
        if (submitTime > deadlineTime) {
            throw ResubmissionClosedException("Resubmission is not allowed after the effective deadline");
        }

        // Resubmission before deadline - add new version
        // State stays Submitted (no change to penalty for resubmission)

    } catch (const NotFoundException&) {
        // First submission - will create new submission record
    }

    // Create submission version
    SubmissionVersion version(submissionId, nextVersion, content, now);

    std::unique_ptr<TransactionGuard> transaction;
    if (m_database) transaction = std::make_unique<TransactionGuard>(*m_database);

    if (!hasExisting) {
        // Create new submission
        Submission submission(0, assignmentId, studentId, SubmissionState::NotSubmitted);
        submission.setFirstSubmittedAt(now);

        // Determine state and penalty
        if (submitTime > deadlineTime) {
            submission.setState(SubmissionState::Late);

            // Calculate penalty
            auto effectiveAssignment = *assignment;
            effectiveAssignment.setDeadline(effectiveDeadline);
            double penalty = m_penaltyStrategy->calculatePenalty(submission, effectiveAssignment, m_clock);
            submission.setPenaltyPercent(penalty);

            // Publish late submission event
            (void)submission;
        } else {
            submission.setState(SubmissionState::Submitted);
        }

        // Save submission to get ID
        submissionId = m_submissionRepo.save(submission);
        version.setSubmissionId(submissionId);
    }

    // Save version
    m_versionRepo.save(version);
    if (!hasExisting && m_events) {
        auto saved = m_submissionRepo.getById(submissionId);
        if (saved->getState() == SubmissionState::Late)
            m_events->publish(LateSubmissionEvent(submissionId, studentId, assignmentId));
    }
    if (transaction) transaction->commit();
}

std::string StudentService::getEffectiveDeadline(int assignmentId, int studentId) {
    // Check for extension
    auto extension = m_extensionRepo.getByAssignmentAndStudent(assignmentId, studentId);
    if (extension) {
        return extension->getNewDeadline();
    }

    // No extension, use assignment deadline
    auto assignment = m_assignmentRepo.getById(assignmentId);
    return assignment->getDeadline();
}

std::string StudentService::stateToString(SubmissionState state) {
    switch (state) {
        case SubmissionState::NotSubmitted: return "NotSubmitted";
        case SubmissionState::Submitted: return "Submitted";
        case SubmissionState::Late: return "Late";
        case SubmissionState::Marked: return "Marked";
        case SubmissionState::Moderated: return "Moderated";
        case SubmissionState::Released: return "Released";
        default: return "Unknown";
    }
}

int StudentService::calculateDaysLate(const std::string& submittedAt, const std::string& deadline) {
    auto submittedTime = TimeUtils::fromIsoString(submittedAt);
    auto deadlineTime = TimeUtils::fromIsoString(deadline);

    const auto duration = std::chrono::duration_cast<std::chrono::seconds>(submittedTime - deadlineTime);
    const auto secondsLate = duration.count();
    if (secondsLate <= 0) {
        return 0;
    }
    constexpr long long kSecondsPerDay = 24 * 60 * 60;
    return static_cast<int>((secondsLate + kSecondsPerDay - 1) / kSecondsPerDay);
}

} // namespace Lms
