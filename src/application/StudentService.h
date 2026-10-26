#pragma once

#include "domain/Entities.h"
#include "domain/User.h"
#include "data/CourseRepository.h"
#include "data/AssignmentRepository.h"
#include "data/SubmissionRepository.h"
#include "data/SubmissionVersionRepository.h"
#include "data/ExtensionRepository.h"
#include "data/DepartmentRepository.h"
#include "data/UserRepository.h"
#include "util/Clock.h"
#include <memory>
#include <vector>
#include <string>

namespace Lms {

/**
 * DTO for assignment information shown to students.
 */
struct AssignmentInfo {
    int id;
    std::string title;
    std::string deadline;
    std::string effectiveDeadline;
    bool hasExtension;
    double maxMark;
    std::string state; // "NotSubmitted", "Submitted", "Late", "Marked", "Moderated", "Released"
    int versionCount;
};

/**
 * DTO for course information shown to students.
 */
struct CourseInfo {
    int id;
    std::string code;
    std::string title;
    std::string department;
    std::vector<std::string> lecturers;
    std::vector<AssignmentInfo> assignments;
};

/**
 * DTO for submission information shown to students.
 */
struct SubmissionInfo {
    int id;
    std::string courseCode;
    std::string courseTitle;
    std::string assignmentTitle;
    std::string state;
    std::string firstSubmittedAt;
    int latestVersion;
    int daysLate;
    bool markReleased;
    double mark;
    std::string feedback;
};

/**
 * Service for student-specific operations.
 */
class StudentService {
public:
    StudentService(ICourseRepository& courseRepo,
                  IAssignmentRepository& assignmentRepo,
                  ISubmissionRepository& submissionRepo,
                  ISubmissionVersionRepository& versionRepo,
                  IExtensionRepository& extensionRepo,
                  IDepartmentRepository& departmentRepo,
                  IUserRepository& userRepo,
                  const Clock& clock);

    /**
     * Get all courses for a student with their assignments.
     * @param studentId The student ID
     * @return List of courses with assignments
     * @throws AuthorisationException if the student is not enrolled in any returned course
     */
    std::vector<CourseInfo> listCourses(int studentId);

    /**
     * Get all submissions for a student.
     * @param studentId The student ID
     * @return List of submissions with course and assignment info
     * @throws AuthorisationException if submissions don't belong to the student
     */
    std::vector<SubmissionInfo> listSubmissions(int studentId);

    /**
     * Submit an assignment.
     * @param studentId The student ID
     * @param assignmentId The assignment ID
     * @param content The submission content
     * @throws AuthorisationException if student is not enrolled in the course
     * @throws ResubmissionClosedException if resubmission is not allowed
     * @throws ValidationException if content is empty or too long
     */
    void submit(int studentId, int assignmentId, const std::string& content);

private:
    ICourseRepository& m_courseRepo;
    IAssignmentRepository& m_assignmentRepo;
    ISubmissionRepository& m_submissionRepo;
    ISubmissionVersionRepository& m_versionRepo;
    IExtensionRepository& m_extensionRepo;
    IDepartmentRepository& m_departmentRepo;
    IUserRepository& m_userRepo;
    const Clock& m_clock;

    /**
     * Get the effective deadline for a student (considering extensions).
     */
    std::string getEffectiveDeadline(int assignmentId, int studentId);

    /**
     * Convert SubmissionState to string.
     */
    std::string stateToString(SubmissionState state);

    /**
     * Calculate days late for a submission.
     */
    int calculateDaysLate(const std::string& submittedAt, const std::string& deadline);
};

} // namespace Lms
