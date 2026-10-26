#include "User.h"
#include "LmsException.h"
#include <stdexcept>

namespace Lms {

Student::Student(int id, const std::string& username)
    : User(id, username) {}

Role Student::getRole() const {
    return Role::Student;
}

bool Student::canPerform(Action action) const {
    switch (action) {
        case Action::Login:
        case Action::ChangePassword:
        case Action::SubmitAssignment:
        case Action::ViewSubmission:
        case Action::ViewOwnCourses:
            return true;
        default:
            return false;
    }
}

Lecturer::Lecturer(int id, const std::string& username, int departmentId)
    : User(id, username), m_departmentId(departmentId) {}

Role Lecturer::getRole() const {
    return Role::Lecturer;
}

bool Lecturer::canPerform(Action action) const {
    switch (action) {
        case Action::Login:
        case Action::ChangePassword:
        case Action::CreateAssignment:
        case Action::EditAssignment:
        case Action::DeleteAssignment:
        case Action::GrantExtension:
        case Action::MarkSubmission:
        case Action::ViewSubmission:
        case Action::ViewOwnCourses:
            return true;
        default:
            return false;
    }
}

HeadOfDepartment::HeadOfDepartment(int id, const std::string& username, int departmentId)
    : User(id, username), m_departmentId(departmentId) {}

Role HeadOfDepartment::getRole() const {
    return Role::HeadOfDepartment;
}

bool HeadOfDepartment::canPerform(Action action) const {
    switch (action) {
        case Action::Login:
        case Action::ChangePassword:
        case Action::ModerateSubmission:
        case Action::ReleaseMarks:
        case Action::ViewSubmission:
        case Action::ViewOwnCourses:
            return true;
        default:
            return false;
    }
}

Administrator::Administrator(int id, const std::string& username)
    : User(id, username) {}

Role Administrator::getRole() const {
    return Role::Administrator;
}

bool Administrator::canPerform(Action action) const {
    switch (action) {
        case Action::Login:
        case Action::ChangePassword:
        case Action::CreateUser:
        case Action::EditUser:
        case Action::DeleteUser:
        case Action::CreateDepartment:
        case Action::EditDepartment:
        case Action::DeleteDepartment:
        case Action::CreateCourse:
        case Action::EditCourse:
        case Action::DeleteCourse:
        case Action::AssignLecturer:
        case Action::EnrollStudent:
        case Action::ViewOwnCourses:
            return true;
        default:
            return false;
    }
}

std::unique_ptr<User> UserFactory::create(Role role, int id, const std::string& username, int departmentId) {
    switch (role) {
        case Role::Student:
            return std::make_unique<Student>(id, username);
        case Role::Lecturer:
            return std::make_unique<Lecturer>(id, username, departmentId);
        case Role::HeadOfDepartment:
            return std::make_unique<HeadOfDepartment>(id, username, departmentId);
        case Role::Administrator:
            return std::make_unique<Administrator>(id, username);
        default:
            throw ValidationException("Invalid role");
    }
}

} // namespace Lms
