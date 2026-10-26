#pragma once

#include <string>
#include <memory>

namespace Lms {

/**
 * User roles in the system.
 */
enum class Role {
    Student,
    Lecturer,
    HeadOfDepartment,
    Administrator
};

/**
 * Actions that can be performed in the system.
 */
enum class Action {
    // User management
    CreateUser,
    EditUser,
    DeleteUser,
    ChangePassword,

    // Department management
    CreateDepartment,
    EditDepartment,
    DeleteDepartment,

    // Course management
    CreateCourse,
    EditCourse,
    DeleteCourse,
    AssignLecturer,
    EnrollStudent,

    // Assignment management
    CreateAssignment,
    EditAssignment,
    DeleteAssignment,
    GrantExtension,

    // Submission and marking
    SubmitAssignment,
    MarkSubmission,
    ModerateSubmission,
    ReleaseMarks,
    ViewSubmission,

    // General
    Login,
    ViewOwnCourses
};

/**
 * Abstract base class for all users.
 * Uses polymorphism for role-based access control.
 */
class User {
public:
    virtual ~User() = default;

    /**
     * Get the user's role.
     */
    virtual Role getRole() const = 0;

    /**
     * Check if this user can perform the given action.
     */
    virtual bool canPerform(Action action) const = 0;

    /**
     * Get the user's ID.
     */
    int getId() const { return m_id; }

    /**
     * Get the username.
     */
    const std::string& getUsername() const { return m_username; }

    /**
     * Set the user's ID.
     */
    void setId(int id) { m_id = id; }

protected:
    User(int id, const std::string& username)
        : m_id(id), m_username(username) {}

    int m_id;
    std::string m_username;
};

/**
 * Student user - can submit assignments and view released marks.
 */
class Student : public User {
public:
    Student(int id, const std::string& username);
    Role getRole() const override;
    bool canPerform(Action action) const override;
};

/**
 * Lecturer user - can create assignments, grant extensions, mark submissions.
 */
class Lecturer : public User {
public:
    Lecturer(int id, const std::string& username, int departmentId);
    Role getRole() const override;
    bool canPerform(Action action) const override;

    int getDepartmentId() const { return m_departmentId; }

private:
    int m_departmentId;
};

/**
 * Head of Department - can moderate marks for their department.
 */
class HeadOfDepartment : public User {
public:
    HeadOfDepartment(int id, const std::string& username, int departmentId);
    Role getRole() const override;
    bool canPerform(Action action) const override;

    int getDepartmentId() const { return m_departmentId; }

private:
    int m_departmentId;
};

/**
 * Administrator - can manage users, departments, courses, and enrolments.
 */
class Administrator : public User {
public:
    Administrator(int id, const std::string& username);
    Role getRole() const override;
    bool canPerform(Action action) const override;
};

/**
 * Factory for creating User objects based on role.
 */
class UserFactory {
public:
    /**
     * Create a user object based on the specified role.
     */
    static std::unique_ptr<User> create(Role role, int id, const std::string& username, int departmentId = 0);
};

} // namespace Lms
