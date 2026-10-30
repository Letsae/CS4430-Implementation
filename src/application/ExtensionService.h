#pragma once

#include "data/AssignmentRepository.h"
#include "data/CourseRepository.h"
#include "data/Database.h"
#include "data/EnrolmentRepository.h"
#include "data/ExtensionRepository.h"
#include "data/UserRepository.h"
#include "domain/Observer.h"
#include "util/Clock.h"

namespace Lms {

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
};

} // namespace Lms
