#include "ExtensionService.h"
#include "domain/LmsException.h"

#include <algorithm>
#include <cctype>

namespace Lms {

ExtensionService::ExtensionService(Database& database, IAssignmentRepository& assignments,
        ICourseRepository& courses, IEnrolmentRepository& enrolments,
        IExtensionRepository& extensions, IUserRepository& users,
        EventPublisher& events, const Clock& clock)
    : m_database(database), m_assignments(assignments), m_courses(courses),
      m_enrolments(enrolments), m_extensions(extensions), m_users(users),
      m_events(events), m_clock(clock) {}

void ExtensionService::requireLecturer(const User& lecturer) const {
    if (lecturer.getRole() != Role::Lecturer || !lecturer.canPerform(Action::GrantExtension))
        throw AuthorisationException("Only lecturers can grant extensions");
    if (!m_users.isActive(lecturer.getId())) throw AuthorisationException("Lecturer account is inactive");
}

std::unique_ptr<Assignment> ExtensionService::requireOwnedAssignment(const User& lecturer, int assignmentId) const {
    requireLecturer(lecturer);
    auto assignment = m_assignments.getById(assignmentId);
    const auto courses = m_courses.getByLecturer(lecturer.getId());
    if (std::none_of(courses.begin(), courses.end(), [&assignment](const Course& course) {
            return course.getId() == assignment->getCourseId();
        }))
        throw AuthorisationException("Assignment is outside your assigned courses");
    return assignment;
}

std::vector<Assignment> ExtensionService::listAssignments(const User& lecturer) const {
    requireLecturer(lecturer);
    std::vector<Assignment> result;
    for (const auto& course : m_courses.getByLecturer(lecturer.getId())) {
        auto items = m_assignments.getByCourse(course.getId());
        result.insert(result.end(), items.begin(), items.end());
    }
    return result;
}

std::vector<std::unique_ptr<User>> ExtensionService::listEnrolledStudents(
        const User& lecturer, int assignmentId) const {
    auto assignment = requireOwnedAssignment(lecturer, assignmentId);
    auto students = m_enrolments.getStudentsForCourse(assignment->getCourseId());
    students.erase(std::remove_if(students.begin(), students.end(), [this](const auto& student) {
        return student->getRole() != Role::Student || !m_users.isActive(student->getId());
    }), students.end());
    return students;
}

std::string ExtensionService::effectiveDeadline(int assignmentId, int studentId) const {
    auto extension = m_extensions.getByAssignmentAndStudent(assignmentId, studentId);
    if (extension) return extension->getNewDeadline();
    return m_assignments.getById(assignmentId)->getDeadline();
}

std::chrono::system_clock::time_point ExtensionService::validatedProposedDeadline(
        const std::string& newDeadline, const std::string& reason) const {
    if (newDeadline.empty()) throw ValidationException("New deadline is required");
    if (reason.empty() || std::all_of(reason.begin(), reason.end(), [](unsigned char c) { return std::isspace(c); }))
        throw ValidationException("Extension reason is required");
    return TimeUtils::fromIsoString(newDeadline);
}

void ExtensionService::grantExtension(const User& lecturer, int assignmentId, int studentId,
        const std::string& newDeadline, const std::string& reason) {
    auto assignment = requireOwnedAssignment(lecturer, assignmentId);
    if (!m_users.exists(studentId)) throw NotFoundException("Student not found with ID: " + std::to_string(studentId));
    auto student = m_users.getById(studentId);
    if (student->getRole() != Role::Student) throw ValidationException("Selected user is not a student");
    if (!m_users.isActive(studentId)) throw ValidationException("Student account is inactive");
    if (!m_enrolments.isEnrolled(assignment->getCourseId(), studentId))
        throw AuthorisationException("Student is not enrolled in this course");
    const auto proposed = validatedProposedDeadline(newDeadline, reason);
    const auto current = TimeUtils::fromIsoString(effectiveDeadline(assignmentId, studentId));
    if (proposed <= current) throw ValidationException("New deadline must be later than the current effective deadline");

    auto existing = m_extensions.getByAssignmentAndStudent(assignmentId, studentId);
    const bool updated = static_cast<bool>(existing);
    Extension extension = existing ? *existing : Extension(0, assignmentId, studentId, newDeadline, reason, lecturer.getId());
    extension.setNewDeadline(newDeadline);
    extension.setReason(reason);
    extension.setGrantedBy(lecturer.getId());

    TransactionGuard transaction(m_database);
    m_extensions.save(extension);
    m_events.publish(ExtensionGrantedEvent(studentId, assignmentId, newDeadline, reason, updated));
    transaction.commit();
}

ClassExtensionResult ExtensionService::grantClassExtension(const User& lecturer, int assignmentId,
        const std::string& newDeadline, const std::string& reason) {
    auto assignment = requireOwnedAssignment(lecturer, assignmentId);
    const auto proposed = validatedProposedDeadline(newDeadline, reason);

    struct Planned { int studentId; std::unique_ptr<Extension> existing; };
    std::vector<Planned> plan;
    ClassExtensionResult result;
    for (const auto& student : m_enrolments.getStudentsForCourse(assignment->getCourseId())) {
        if (student->getRole() != Role::Student) continue;
        if (!m_users.isActive(student->getId())) { ++result.skippedInactive; continue; }
        auto existing = m_extensions.getByAssignmentAndStudent(assignmentId, student->getId());
        const auto current = TimeUtils::fromIsoString(
            existing ? existing->getNewDeadline() : assignment->getDeadline());
        if (proposed <= current) { ++result.skippedAlreadyLater; continue; }
        plan.push_back({student->getId(), std::move(existing)});
    }
    if (plan.empty())
        throw ValidationException("No enrolled student would receive a later deadline");

    TransactionGuard transaction(m_database);
    for (auto& item : plan) {
        const bool updated = static_cast<bool>(item.existing);
        Extension extension = updated ? *item.existing
            : Extension(0, assignmentId, item.studentId, newDeadline, reason, lecturer.getId());
        extension.setNewDeadline(newDeadline);
        extension.setReason(reason);
        extension.setGrantedBy(lecturer.getId());
        m_extensions.save(extension);
        m_events.publish(ExtensionGrantedEvent(item.studentId, assignmentId, newDeadline, reason, updated));
        if (updated) ++result.updated; else ++result.granted;
    }
    transaction.commit();
    return result;
}

} // namespace Lms
