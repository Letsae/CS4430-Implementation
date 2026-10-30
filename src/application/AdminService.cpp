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
        IPasswordHasher& hasher, Database& database, EventPublisher& events, const Clock& clock)
    : AdminService(userRepo, departmentRepo, courseRepo, enrolmentRepo, hasher) {
    m_database = &database;
    m_events = &events;
    m_clock = &clock;
}

void AdminService::requireAdministrator(const User& administrator, Action action) {
    if (administrator.getRole() != Role::Administrator || !administrator.canPerform(action))
        throw AuthorisationException("Only administrators can perform this action");
}

int AdminService::createUser(const User& administrator, const std::string& username, Role role,
        const std::string& password, int departmentId) {
    requireAdministrator(administrator, Action::CreateUser);
    return createUserCore(username, role, password, departmentId);
}

int AdminService::createSeedUser(const std::string& username, Role role, const std::string& password, int departmentId) {
    return createUserCore(username, role, password, departmentId);
}

int AdminService::createUserCore(const std::string& username, Role role, const std::string& password, int departmentId) {
    if (username.empty() || username.size() > 100) throw ValidationException("Username must be 1 to 100 characters");
    if (role == Role::Lecturer && departmentId <= 0) throw ValidationException("Lecturer must be assigned to a department");
    // Validate username doesn't exist
    if (m_userRepo.usernameExists(username)) {
        throw ValidationException("Username already exists");
    }

    // Validate department assignment for roles that require one before the insert,
    // so invalid or conflicting assignments are reported as user errors rather
    // than opaque SQLite constraint failures.
    if (role == Role::HeadOfDepartment) {
        if (departmentId <= 0) {
            throw ValidationException("Head of Department must be assigned to a department");
        }
        if (!m_departmentRepo.exists(departmentId)) {
            throw NotFoundException("Department not found with ID: " + std::to_string(departmentId));
        }
        for (const auto& user : m_userRepo.getByDepartment(departmentId)) {
            if (user->getRole() == Role::HeadOfDepartment) {
                throw ValidationException("This department already has a Head of Department");
            }
        }
    }

    if (role != Role::Student && role != Role::Lecturer && role != Role::HeadOfDepartment && role != Role::Administrator)
        throw ValidationException("Invalid user role");
    if (role == Role::Lecturer && !m_departmentRepo.exists(departmentId))
        throw NotFoundException("Department not found with ID: " + std::to_string(departmentId));

    auto user = UserFactory::create(UserData{0, username, role, departmentId});
    // Hash password
    std::string passwordHash = m_hasher.hashPassword(password);

    // Save user
    return m_userRepo.saveWithPassword(user->getUsername(), user->getRole(), passwordHash, departmentId);
}

int AdminService::createDepartment(const User& administrator, const std::string& name) {
    requireAdministrator(administrator, Action::CreateDepartment);
    return createDepartmentCore(name);
}

int AdminService::createSeedDepartment(const std::string& name) {
    return createDepartmentCore(name);
}

int AdminService::createDepartmentCore(const std::string& name) {
    const auto nonSpace = std::find_if_not(name.begin(), name.end(), [](unsigned char c) { return std::isspace(c); });
    if (name.empty() || nonSpace == name.end() || name.size() > 100) {
        throw ValidationException("Department name must be 1 to 100 non-blank characters");
    }
    for (const auto& department : m_departmentRepo.getAll()) {
        if (department.getName() == name) throw ValidationException("Department already exists");
    }
    Department dept(0, name);
    return m_departmentRepo.save(dept);
}

int AdminService::createCourse(const User& administrator, const std::string& code,
        const std::string& title, int departmentId) {
    requireAdministrator(administrator, Action::CreateCourse);
    return createCourseCore(code, title, departmentId);
}

int AdminService::createSeedCourse(const std::string& code, const std::string& title, int departmentId) {
    return createCourseCore(code, title, departmentId);
}

int AdminService::createCourseCore(const std::string& code, const std::string& title, int departmentId) {
    const auto nonSpaceCode = std::find_if_not(code.begin(), code.end(), [](unsigned char c) { return std::isspace(c); });
    const auto nonSpaceTitle = std::find_if_not(title.begin(), title.end(), [](unsigned char c) { return std::isspace(c); });
    if (code.empty() || nonSpaceCode == code.end() || code.size() > 20) throw ValidationException("Course code must be 1 to 20 non-blank characters");
    if (title.empty() || nonSpaceTitle == title.end() || title.size() > 100) throw ValidationException("Course title must be 1 to 100 non-blank characters");
    // Validate department exists
    if (!m_departmentRepo.exists(departmentId)) {
        throw NotFoundException("Department not found with ID: " + std::to_string(departmentId));
    }
    for (const auto& course : m_courseRepo.getAll()) {
        if (course.getCode() == code) throw ValidationException("Course code already exists");
    }

    Course course(0, code, title, departmentId);
    return m_courseRepo.save(course);
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

std::vector<AdminCourseInfo> AdminService::listCoursesForAssignment(const User& administrator) {
    requireAdministrator(administrator, Action::AssignLecturer);
    std::vector<AdminCourseInfo> result;
    for (const auto& course : m_courseRepo.getAll()) {
        auto department = m_departmentRepo.getById(course.getDepartmentId());
        result.push_back({course, department->getName(), m_courseRepo.getLecturerUsernames(course.getId())});
    }
    return result;
}

std::vector<std::unique_ptr<User>> AdminService::listActiveLecturers(const User& administrator) {
    requireAdministrator(administrator, Action::AssignLecturer);
    std::vector<std::unique_ptr<User>> active;
    for (auto& lecturer : m_userRepo.getByRole(Role::Lecturer)) {
        if (m_userRepo.isActive(lecturer->getId())) active.push_back(std::move(lecturer));
    }
    return active;
}

std::vector<std::unique_ptr<User>> AdminService::listActiveStudents(const User& administrator) {
    requireAdministrator(administrator, Action::EnrollStudent);
    std::vector<std::unique_ptr<User>> active;
    for (auto& student : m_userRepo.getByRole(Role::Student)) {
        if (m_userRepo.isActive(student->getId())) active.push_back(std::move(student));
    }
    return active;
}

std::vector<Department> AdminService::listDepartments(const User& administrator) {
    requireAdministrator(administrator, Action::CreateCourse);
    return m_departmentRepo.getAll();
}

void AdminService::enrollStudent(const User& administrator, int courseId, int studentId) {
    if (administrator.getRole() != Role::Administrator || !administrator.canPerform(Action::EnrollStudent))
        throw AuthorisationException("Only administrators can enroll students");
    if (!m_clock) throw DatabaseException("Enrollment clock is unavailable");
    if (!m_courseRepo.exists(courseId))
        throw NotFoundException("Course not found with ID: " + std::to_string(courseId));
    if (!m_userRepo.exists(studentId))
        throw NotFoundException("User not found with ID: " + std::to_string(studentId));
    auto student = m_userRepo.getById(studentId);
    if (student->getRole() != Role::Student) throw ValidationException("Selected user is not a student");
    if (!m_userRepo.isActive(studentId)) throw ValidationException("Student account is inactive");
    if (m_enrolmentRepo.isEnrolled(courseId, studentId))
        throw ValidationException("Student is already enrolled in this course");
    m_enrolmentRepo.enrollAt(courseId, studentId, m_clock->now());
}

} // namespace Lms
