#include "StudentService.h"
#include "domain/LmsException.h"
#include "domain/LatePenaltyStrategy.h"
#include "domain/Observer.h"
#include "data/Database.h"
#include <cmath>
#include <chrono>

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

std::vector<CourseInfo> StudentService::listCourses(int studentId) {
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
                assignInfo.versionCount = 1;
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

            // Get version count (simplified)
            info.latestVersion = 1;

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
    // Validate content
    if (content.empty()) {
        throw ValidationException("Submission content cannot be empty");
    }
    if (content.length() > 20000) {
        throw ValidationException("Submission content exceeds maximum length of 20000 characters");
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

    // Note: In a full implementation, we would use TransactionGuard here
    // to ensure both submission and version are saved atomically.
    // For now, we'll save in sequence (version last so submission ID exists)

    if (!hasExisting) {
        // Create new submission
        Submission submission(0, assignmentId, studentId, SubmissionState::NotSubmitted);
        submission.setFirstSubmittedAt(now);

        // Determine state and penalty
        if (submitTime > deadlineTime) {
            submission.setState(SubmissionState::Late);

            // Calculate penalty
            StandardLatePenalty penaltyStrategy;
            double penalty = penaltyStrategy.calculatePenalty(submission, *assignment, m_clock);
            submission.setPenaltyPercent(penalty);

            // Publish late submission event
            LateSubmissionEvent event(0, studentId, assignmentId);
            // Note: EventPublisher would need to be injected into StudentService
            // For now, we'll skip event publishing
        } else {
            submission.setState(SubmissionState::Submitted);
        }

        // Save submission to get ID
        submissionId = m_submissionRepo.save(submission);
        version.setSubmissionId(submissionId);
    }

    // Save version
    m_versionRepo.save(version);
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

    auto duration = std::chrono::duration_cast<std::chrono::hours>(submittedTime - deadlineTime);
    double hoursLate = static_cast<double>(duration.count());

    if (hoursLate <= 0) {
        return 0;
    }

    return static_cast<int>(std::ceil(hoursLate / 24.0));
}

} // namespace Lms
