#pragma once

#include "domain/Entities.h"
#include "domain/User.h"
#include "data/UserRepository.h"
#include "data/DepartmentRepository.h"
#include "data/CourseRepository.h"
#include "data/EnrolmentRepository.h"
#include "security/IPasswordHasher.h"
#include "data/Database.h"
#include "domain/Observer.h"
#include "util/Clock.h"
#include <memory>

namespace Lms {

struct AdminCourseInfo {
    Course course;
    std::string department;
    std::vector<std::string> lecturers;
};

// Forward declaration
class IEnrolmentRepository;

/**
 * Service for administrative operations.
 */
class AdminService {
public:
    AdminService(IUserRepository& userRepo,
                 IDepartmentRepository& departmentRepo,
                 ICourseRepository& courseRepo,
                 IEnrolmentRepository& enrolmentRepo,
                 IPasswordHasher& hasher);
    AdminService(IUserRepository& userRepo,
                 IDepartmentRepository& departmentRepo,
                 ICourseRepository& courseRepo,
                 IEnrolmentRepository& enrolmentRepo,
                 IPasswordHasher& hasher,
                 Database& database,
                 EventPublisher& events,
                 const Clock& clock);

    /**
     * Create a new user.
     * @param username The username
     * @param role The user role
     * @param password The password
     * @param departmentId The department ID (required for HoD, optional for others)
     * @return The new user ID
     */
    int createUser(const User& administrator, const std::string& username, Role role,
                   const std::string& password, int departmentId);
    int createSeedUser(const std::string& username, Role role, const std::string& password, int departmentId);

    /**
     * Create a new department.
     * @param name The department name
     * @return The new department ID
     */
    int createDepartment(const User& administrator, const std::string& name);
    int createSeedDepartment(const std::string& name);

    /**
     * Create a new course.
     * @param code The course code
     * @param title The course title
     * @param departmentId The department ID
     * @return The new course ID
     */
    int createCourse(const User& administrator, const std::string& code, const std::string& title, int departmentId);
    int createSeedCourse(const std::string& code, const std::string& title, int departmentId);

    /**
     * Assign a lecturer to a course.
     */
    void assignLecturerToCourse(const User& administrator, int courseId, int lecturerId);
    std::vector<AdminCourseInfo> listCoursesForAssignment(const User& administrator);
    std::vector<std::unique_ptr<User>> listActiveLecturers(const User& administrator);
    std::vector<std::unique_ptr<User>> listActiveStudents(const User& administrator);
    std::vector<Department> listDepartments(const User& administrator);

    /**
     * Enroll a student in a course.
     */
    void enrollStudent(const User& administrator, int courseId, int studentId);

private:
    int createUserCore(const std::string& username, Role role, const std::string& password, int departmentId);
    int createDepartmentCore(const std::string& name);
    int createCourseCore(const std::string& code, const std::string& title, int departmentId);
    static void requireAdministrator(const User& administrator, Action action);
    IUserRepository& m_userRepo;
    IDepartmentRepository& m_departmentRepo;
    ICourseRepository& m_courseRepo;
    IEnrolmentRepository& m_enrolmentRepo;
    IPasswordHasher& m_hasher;
    Database* m_database = nullptr;
    EventPublisher* m_events = nullptr;
    const Clock* m_clock = nullptr;
};

} // namespace Lms
