#include "AdminService.h"
#include "domain/LmsException.h"
#include <algorithm>
#include <cctype>

namespace Lms {

AdminService::AdminService(IUserRepository& userRepo,
                           IDepartmentRepository& departmentRepo,
                           ICourseRepository& courseRepo,
                           IEnrolmentRepository& enrolmentRepo,
                           IPasswordHasher& hasher)
    : m_userRepo(userRepo), m_departmentRepo(departmentRepo),
      m_courseRepo(courseRepo), m_enrolmentRepo(enrolmentRepo), m_hasher(hasher) {}

AdminService::AdminService(IUserRepository& userRepo, IDepartmentRepository& departmentRepo,
        ICourseRepository& courseRepo, IEnrolmentRepository& enrolmentRepo,
        IPasswordHasher& hasher, Database& database, EventPublisher& events)
    : AdminService(userRepo, departmentRepo, courseRepo, enrolmentRepo, hasher) {
    m_database = &database;
    m_events = &events;
}

int AdminService::createUser(const std::string& username, Role role, const std::string& password, int departmentId) {
    if (username.empty() || username.size() > 100) throw ValidationException("Username must be 1 to 100 characters");
    if (role == Role::Lecturer && departmentId <= 0) throw ValidationException("Lecturer must be assigned to a department");
    // Validate username doesn't exist
    if (m_userRepo.usernameExists(username)) {
        throw ValidationException("Username already exists");
    }

    // Validate department ID for HoD
    if (role == Role::HeadOfDepartment && departmentId == 0) {
        throw ValidationException("Head of Department must be assigned to a department");
    }

    // Hash password
    std::string passwordHash = m_hasher.hashPassword(password);

    // Save user
    return m_userRepo.saveWithPassword(username, role, passwordHash, departmentId);
}

int AdminService::createDepartment(const std::string& name) {
    const auto nonSpace = std::find_if_not(name.begin(), name.end(), [](unsigned char c) { return std::isspace(c); });
    if (name.empty() || nonSpace == name.end() || name.size() > 100) {
        throw ValidationException("Department name must be 1 to 100 non-blank characters");
    }
    Department dept(0, name);
    return m_departmentRepo.save(dept);
}

int AdminService::createCourse(const std::string& code, const std::string& title, int departmentId) {
    const auto nonSpaceCode = std::find_if_not(code.begin(), code.end(), [](unsigned char c) { return std::isspace(c); });
    const auto nonSpaceTitle = std::find_if_not(title.begin(), title.end(), [](unsigned char c) { return std::isspace(c); });
    if (code.empty() || nonSpaceCode == code.end() || code.size() > 20) throw ValidationException("Course code must be 1 to 20 non-blank characters");
    if (title.empty() || nonSpaceTitle == title.end() || title.size() > 100) throw ValidationException("Course title must be 1 to 100 non-blank characters");
    // Validate department exists
    if (!m_departmentRepo.exists(departmentId)) {
        throw NotFoundException("Department not found with ID: " + std::to_string(departmentId));
    }

    Course course(0, code, title, departmentId);
    return m_courseRepo.save(course);
}

void AdminService::assignLecturer(int courseId, int lecturerId) {
    // Validate course exists
    if (!m_courseRepo.exists(courseId)) {
        throw NotFoundException("Course not found with ID: " + std::to_string(courseId));
    }

    // Validate lecturer exists
    if (!m_userRepo.exists(lecturerId)) {
        throw NotFoundException("User not found with ID: " + std::to_string(lecturerId));
    }

    // Insert into course_lecturer table
    // Note: This would require a dedicated repository or adding to CourseRepository
    // For now, we'll use direct SQL
    // Implementation would go here
    throw DatabaseException("assignLecturer not fully implemented");
}

void AdminService::assignLecturerToCourse(const User& administrator, int courseId, int lecturerId) {
    if (administrator.getRole() != Role::Administrator || !administrator.canPerform(Action::AssignLecturer)) {
        throw AuthorisationException("Only administrators can assign lecturers to courses");
    }
    if (!m_database || !m_events) throw DatabaseException("Assignment service dependencies are unavailable");
    if (!m_courseRepo.exists(courseId)) throw NotFoundException("Course not found with ID: " + std::to_string(courseId));
    if (!m_userRepo.exists(lecturerId)) throw NotFoundException("User not found with ID: " + std::to_string(lecturerId));

    auto course = m_courseRepo.getById(courseId);
    auto lecturer = m_userRepo.getById(lecturerId);
    if (!m_userRepo.isActive(lecturerId)) throw ValidationException("Lecturer account is inactive");
    if (lecturer->getRole() != Role::Lecturer) throw ValidationException("Selected user is not a lecturer");
    const auto* lecturerRole = dynamic_cast<const Lecturer*>(lecturer.get());
    if (!lecturerRole || lecturerRole->getDepartmentId() != course->getDepartmentId()) {
        throw ValidationException("Lecturer and course must belong to the same department");
    }
    for (const auto& assignedCourse : m_courseRepo.getByLecturer(lecturerId)) {
        if (assignedCourse.getId() == courseId) throw ValidationException("Lecturer is already assigned to this course");
    }

    TransactionGuard transaction(*m_database);
    m_courseRepo.assignLecturer(courseId, lecturerId);
    LecturerAssignedEvent event(lecturerId, courseId, course->getCode());
    // Dispatch synchronously before commit so notification inserts share this transaction and failure rolls back the link.
    m_events->publish(event);
    transaction.commit();
}

std::vector<AdminCourseInfo> AdminService::listCoursesForAssignment() {
    std::vector<AdminCourseInfo> result;
    for (const auto& course : m_courseRepo.getAll()) {
        auto department = m_departmentRepo.getById(course.getDepartmentId());
        result.push_back({course, department->getName(), m_courseRepo.getLecturerUsernames(course.getId())});
    }
    return result;
}

std::vector<std::unique_ptr<User>> AdminService::listActiveLecturers() {
    std::vector<std::unique_ptr<User>> active;
    for (auto& lecturer : m_userRepo.getByRole(Role::Lecturer)) {
        if (m_userRepo.isActive(lecturer->getId())) active.push_back(std::move(lecturer));
    }
    return active;
}

std::vector<Department> AdminService::listDepartments() { return m_departmentRepo.getAll(); }

void AdminService::enrollStudent(int courseId, int studentId) {
    // Validate course exists
    if (!m_courseRepo.exists(courseId)) {
        throw NotFoundException("Course not found with ID: " + std::to_string(courseId));
    }

    // Validate student exists
    if (!m_userRepo.exists(studentId)) {
        throw NotFoundException("User not found with ID: " + std::to_string(studentId));
    }

    m_enrolmentRepo.enroll(courseId, studentId);
}

} // namespace Lms
