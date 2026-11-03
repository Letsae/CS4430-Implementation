#pragma once

#include "data/AssignmentRepository.h"
#include "data/CourseRepository.h"
#include "data/Database.h"
#include "data/EnrolmentRepository.h"
#include "data/ExtensionRepository.h"
#include "data/UserRepository.h"
#include "domain/Observer.h"
#include "util/Clock.h"

#include <chrono>
#include <string>
#include <vector>

namespace Lms {

/**
 * Outcome of a class-wide extension (CR1, v1.1).
 * Students whose effective deadline is already the same or later are skipped, never shortened.
 */
struct ClassExtensionResult {
    int granted = 0;             // students given a new extension
    int updated = 0;             // students whose existing extension was moved later
    int skippedAlreadyLater = 0; // effective deadline already >= the new deadline
    int skippedInactive = 0;     // inactive student accounts (ignored)

    int affected() const { return granted + updated; }
};

class ExtensionService {
public:
    ExtensionService(Database& database, IAssignmentRepository& assignments,
                     ICourseRepository& courses, IEnrolmentRepository& enrolments,
                     IExtensionRepository& extensions, IUserRepository& users,
                     EventPublisher& events, const Clock& clock);

    std::vector<Assignment> listAssignments(const User& lecturer) const;
    std::vector<std::unique_ptr<User>> listEnrolledStudents(const User& lecturer, int assignmentId) const;
    void grantExtension(const User& lecturer, int assignmentId, int studentId,
                        const std::string& newDeadline, const std::string& reason);

    /**
     * CR1 (v1.1): extend the deadline of one assignment for every active student enrolled in its
     * course. Atomic: either all affected students receive the extension or none do.
     */
    ClassExtensionResult grantClassExtension(const User& lecturer, int assignmentId,
                                             const std::string& newDeadline, const std::string& reason);

private:
    Database& m_database;
    IAssignmentRepository& m_assignments;
    ICourseRepository& m_courses;
    IEnrolmentRepository& m_enrolments;
    IExtensionRepository& m_extensions;
    IUserRepository& m_users;
    EventPublisher& m_events;
    const Clock& m_clock;

    void requireLecturer(const User& lecturer) const;
    std::unique_ptr<Assignment> requireOwnedAssignment(const User& lecturer, int assignmentId) const;
    std::string effectiveDeadline(int assignmentId, int studentId) const;
    std::chrono::system_clock::time_point validatedProposedDeadline(const std::string& newDeadline,
                                                                    const std::string& reason) const;
};

} // namespace Lms
