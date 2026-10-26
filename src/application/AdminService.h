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
                 EventPublisher& events);

    /**
     * Create a new user.
     * @param username The username
     * @param role The user role
     * @param password The password
     * @param departmentId The department ID (required for HoD, optional for others)
     * @return The new user ID
     */
    int createUser(const std::string& username, Role role, const std::string& password, int departmentId);

    /**
     * Create a new department.
     * @param name The department name
     * @return The new department ID
     */
    int createDepartment(const std::string& name);

    /**
     * Create a new course.
     * @param code The course code
     * @param title The course title
     * @param departmentId The department ID
     * @return The new course ID
     */
    int createCourse(const std::string& code, const std::string& title, int departmentId);

    /**
     * Assign a lecturer to a course.
     */
    void assignLecturer(int courseId, int lecturerId);
    void assignLecturerToCourse(const User& administrator, int courseId, int lecturerId);
    std::vector<AdminCourseInfo> listCoursesForAssignment();
    std::vector<std::unique_ptr<User>> listActiveLecturers();
    std::vector<Department> listDepartments();

    /**
     * Enroll a student in a course.
     */
    void enrollStudent(int courseId, int studentId);

private:
    IUserRepository& m_userRepo;
    IDepartmentRepository& m_departmentRepo;
    ICourseRepository& m_courseRepo;
    IEnrolmentRepository& m_enrolmentRepo;
    IPasswordHasher& m_hasher;
    Database* m_database = nullptr;
    EventPublisher* m_events = nullptr;
};

} // namespace Lms
