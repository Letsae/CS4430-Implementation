#pragma once

#include "data/CourseRepository.h"
#include "data/AssignmentRepository.h"
#include "data/SubmissionRepository.h"
#include "data/SubmissionVersionRepository.h"
#include "data/ExtensionRepository.h"
#include "data/DepartmentRepository.h"
#include "data/UserRepository.h"
#include "data/EnrolmentRepository.h"
#include "domain/LmsException.h"
#include "domain/User.h"
#include "security/IPasswordHasher.h"
#include <algorithm>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace Lms {
namespace Test {

class FakeCourseRepository : public ICourseRepository {
public:
    std::vector<Course> courses;
    std::map<int, std::vector<int>> studentCourseIds;
    std::map<int, std::vector<int>> lecturerCourseIds;
    std::map<int, std::vector<std::string>> lecturerNames;

    std::unique_ptr<Course> getById(int id) override {
        for (const auto& c : courses) {
            if (c.getId() == id) {
                return std::make_unique<Course>(c);
            }
        }
        throw NotFoundException("Course not found");
    }

    std::vector<Course> getAll() override { return courses; }

    int save(const Course& entity) override {
        Course copy = entity;
        if (copy.getId() == 0) {
            copy.setId(static_cast<int>(courses.size()) + 1);
        }
        courses.push_back(copy);
        return copy.getId();
    }

    void remove(int id) override {
        courses.erase(std::remove_if(courses.begin(), courses.end(),
                                     [id](const Course& c) { return c.getId() == id; }),
                      courses.end());
    }

    bool exists(int id) override {
        return std::any_of(courses.begin(), courses.end(),
                           [id](const Course& c) { return c.getId() == id; });
    }

    std::vector<Course> getByDepartment(int departmentId) override {
        std::vector<Course> result;
        for (const auto& c : courses) {
            if (c.getDepartmentId() == departmentId) {
                result.push_back(c);
            }
        }
        return result;
    }

    std::vector<Course> getByStudent(int studentId) override {
        std::vector<Course> result;
        auto it = studentCourseIds.find(studentId);
        if (it == studentCourseIds.end()) {
            return result;
        }
        for (int courseId : it->second) {
            for (const auto& c : courses) {
                if (c.getId() == courseId) {
                    result.push_back(c);
                }
            }
        }
        return result;
    }

    std::vector<Course> getByLecturer(int lecturerId) override {
        std::vector<Course> result;
        auto it = lecturerCourseIds.find(lecturerId);
        if (it == lecturerCourseIds.end()) return result;
        for (int courseId : it->second) {
            for (const auto& course : courses) if (course.getId() == courseId) result.push_back(course);
        }
        return result;
    }

    std::vector<std::string> getLecturerUsernames(int courseId) override {
        auto it = lecturerNames.find(courseId);
        if (it == lecturerNames.end()) {
            return {};
        }
        return it->second;
    }

    void assignLecturer(int courseId, int lecturerId) override {
        lecturerCourseIds[lecturerId].push_back(courseId);
    }

    void enroll(int studentId, int courseId) {
        studentCourseIds[studentId].push_back(courseId);
    }
};

class FakeAssignmentRepository : public IAssignmentRepository {
public:
    std::vector<Assignment> assignments;

    std::unique_ptr<Assignment> getById(int id) override {
        for (const auto& a : assignments) {
            if (a.getId() == id) {
                return std::make_unique<Assignment>(a);
            }
        }
        throw NotFoundException("Assignment not found");
    }

    std::vector<Assignment> getAll() override { return assignments; }

    int save(const Assignment& entity) override {
        Assignment copy = entity;
        if (copy.getId() == 0) {
            copy.setId(static_cast<int>(assignments.size()) + 1);
        }
        assignments.push_back(copy);
        return copy.getId();
    }

    void remove(int id) override {
        assignments.erase(std::remove_if(assignments.begin(), assignments.end(),
                                         [id](const Assignment& a) { return a.getId() == id; }),
                          assignments.end());
    }

    bool exists(int id) override {
        return std::any_of(assignments.begin(), assignments.end(),
                           [id](const Assignment& a) { return a.getId() == id; });
    }

    std::vector<Assignment> getByCourse(int courseId) override {
        std::vector<Assignment> result;
        for (const auto& a : assignments) {
            if (a.getCourseId() == courseId) {
                result.push_back(a);
            }
        }
        return result;
    }
};

class FakeSubmissionRepository : public ISubmissionRepository {
public:
    std::vector<Submission> submissions;
    int nextId = 1;

    std::unique_ptr<Submission> getById(int id) override {
        for (const auto& s : submissions) {
            if (s.getId() == id) {
                return std::make_unique<Submission>(s);
            }
        }
        throw NotFoundException("Submission not found");
    }

    std::vector<Submission> getAll() override { return submissions; }

    int save(const Submission& entity) override {
        Submission copy = entity;
        if (copy.getId() == 0) {
            copy.setId(nextId++);
        }
        submissions.push_back(copy);
        return copy.getId();
    }

    void remove(int id) override {
        submissions.erase(std::remove_if(submissions.begin(), submissions.end(),
                                         [id](const Submission& s) { return s.getId() == id; }),
                          submissions.end());
    }

    bool exists(int id) override {
        return std::any_of(submissions.begin(), submissions.end(),
                           [id](const Submission& s) { return s.getId() == id; });
    }

    std::vector<Submission> getByAssignment(int assignmentId) override {
        std::vector<Submission> result;
        for (const auto& s : submissions) {
            if (s.getAssignmentId() == assignmentId) {
                result.push_back(s);
            }
        }
        return result;
    }

    std::vector<Submission> getByStudent(int studentId) override {
        std::vector<Submission> result;
        for (const auto& s : submissions) {
            if (s.getStudentId() == studentId) {
                result.push_back(s);
            }
        }
        return result;
    }

    std::unique_ptr<Submission> getByAssignmentAndStudent(int assignmentId, int studentId) override {
        for (const auto& s : submissions) {
            if (s.getAssignmentId() == assignmentId && s.getStudentId() == studentId) {
                return std::make_unique<Submission>(s);
            }
        }
        throw NotFoundException("Submission not found");
    }

    std::vector<Submission> getByState(SubmissionState state) override {
        std::vector<Submission> result;
        for (const auto& s : submissions) {
            if (s.getState() == state) {
                result.push_back(s);
            }
        }
        return result;
    }
};

class FakeExtensionRepository : public IExtensionRepository {
public:
    std::vector<Extension> extensions;

    std::unique_ptr<Extension> getById(int id) override {
        for (const auto& e : extensions) {
            if (e.getId() == id) {
                return std::make_unique<Extension>(e);
            }
        }
        throw NotFoundException("Extension not found");
    }

    std::vector<Extension> getAll() override { return extensions; }

    int save(const Extension& entity) override {
        Extension copy = entity;
        if (copy.getId() == 0) {
            copy.setId(static_cast<int>(extensions.size()) + 1);
        }
        extensions.push_back(copy);
        return copy.getId();
    }

    std::unique_ptr<Extension> getByAssignmentAndStudent(int assignmentId, int studentId) override {
        for (const auto& e : extensions) {
            if (e.getAssignmentId() == assignmentId && e.getStudentId() == studentId) {
                return std::make_unique<Extension>(e);
            }
        }
        return nullptr;
    }

    std::vector<Extension> getByAssignment(int assignmentId) override {
        std::vector<Extension> result;
        for (const auto& e : extensions) {
            if (e.getAssignmentId() == assignmentId) {
                result.push_back(e);
            }
        }
        return result;
    }
};

class FakeSubmissionVersionRepository : public ISubmissionVersionRepository {
public:
    std::vector<SubmissionVersion> versions;
    std::map<int, int> latestVersions; // submissionId -> latest version

    std::vector<SubmissionVersion> getBySubmission(int submissionId) override {
        std::vector<SubmissionVersion> result;
        for (const auto& v : versions) {
            if (v.getSubmissionId() == submissionId) {
                result.push_back(v);
            }
        }
        return result;
    }

    std::unique_ptr<SubmissionVersion> get(int submissionId, int versionNo) override {
        for (const auto& v : versions) {
            if (v.getSubmissionId() == submissionId && v.getVersionNo() == versionNo) {
                return std::make_unique<SubmissionVersion>(v);
            }
        }
        throw NotFoundException("Submission version not found");
    }

    int getLatestVersion(int submissionId) override {
        auto it = latestVersions.find(submissionId);
        if (it == latestVersions.end()) {
            return 0;
        }
        return it->second;
    }

    void save(const SubmissionVersion& version) override {
        versions.push_back(version);
        latestVersions[version.getSubmissionId()] = version.getVersionNo();
    }
};

class FakeDepartmentRepository : public IDepartmentRepository {
public:
    std::vector<Department> departments;

    std::unique_ptr<Department> getById(int id) override {
        for (const auto& d : departments) {
            if (d.getId() == id) {
                return std::make_unique<Department>(d);
            }
        }
        throw NotFoundException("Department not found");
    }

    std::vector<Department> getAll() override { return departments; }

    int save(const Department& entity) override {
        Department copy = entity;
        if (copy.getId() == 0) {
            copy.setId(static_cast<int>(departments.size()) + 1);
        }
        departments.push_back(copy);
        return copy.getId();
    }

    void remove(int /* id */) override {}

    bool exists(int id) override {
        return std::any_of(departments.begin(), departments.end(),
                           [id](const Department& d) { return d.getId() == id; });
    }
};

class FakeUserRepository : public IUserRepository {
public:
    struct StoredUser {
        int id;
        std::string username;
        Role role;
        int departmentId;
        std::string passwordHash;
    };

    std::vector<StoredUser> users;
    std::vector<int> inactiveIds;
    int nextId = 1;

    std::unique_ptr<User> getById(int id) override {
        for (const auto& u : users) {
            if (u.id == id) {
                return UserFactory::create(u.role, u.id, u.username, u.departmentId);
            }
        }
        throw NotFoundException("User not found");
    }

    std::vector<std::unique_ptr<User>> getAll() override {
        std::vector<std::unique_ptr<User>> result;
        for (const auto& u : users) {
            result.push_back(UserFactory::create(u.role, u.id, u.username, u.departmentId));
        }
        return result;
    }

    int save(const User& /* entity */) override {
        throw DatabaseException("use saveWithPassword");
    }

    void remove(int /* id */) override {}

    bool exists(int id) override {
        return std::any_of(users.begin(), users.end(),
                           [id](const StoredUser& u) { return u.id == id; });
    }

    bool isActive(int id) override {
        return exists(id) && std::find(inactiveIds.begin(), inactiveIds.end(), id) == inactiveIds.end();
    }

    std::unique_ptr<User> getByUsername(const std::string& username) override {
        for (const auto& u : users) {
            if (u.username == username) {
                return UserFactory::create(u.role, u.id, u.username, u.departmentId);
            }
        }
        throw NotFoundException("User not found");
    }

    std::vector<std::unique_ptr<User>> getByRole(Role role) override {
        std::vector<std::unique_ptr<User>> result;
        for (const auto& u : users) {
            if (u.role == role) {
                result.push_back(UserFactory::create(u.role, u.id, u.username, u.departmentId));
            }
        }
        return result;
    }

    std::vector<std::unique_ptr<User>> getByDepartment(int departmentId) override {
        std::vector<std::unique_ptr<User>> result;
        for (const auto& u : users) {
            if (u.departmentId == departmentId) {
                result.push_back(UserFactory::create(u.role, u.id, u.username, u.departmentId));
            }
        }
        return result;
    }

    bool usernameExists(const std::string& username) override {
        return std::any_of(users.begin(), users.end(),
                           [&username](const StoredUser& u) { return u.username == username; });
    }

    std::string getPasswordHash(const std::string& username) override {
        for (const auto& u : users) {
            if (u.username == username) {
                return u.passwordHash;
            }
        }
        throw NotFoundException("User not found");
    }

    void setPasswordHash(int userId, const std::string& hash) override {
        for (auto& u : users) {
            if (u.id == userId) {
                u.passwordHash = hash;
                return;
            }
        }
        throw NotFoundException("User not found");
    }

    int saveWithPassword(const std::string& username, Role role,
                         const std::string& passwordHash, int departmentId) override {
        StoredUser u{nextId++, username, role, departmentId, passwordHash};
        users.push_back(u);
        return u.id;
    }
};

class FakeEnrolmentRepository : public IEnrolmentRepository {
public:
    FakeCourseRepository& courses;
    explicit FakeEnrolmentRepository(FakeCourseRepository& c) : courses(c) {}

    void enroll(int courseId, int studentId) override {
        courses.enroll(studentId, courseId);
    }

    void unenroll(int courseId, int studentId) override {
        auto& ids = courses.studentCourseIds[studentId];
        ids.erase(std::remove(ids.begin(), ids.end(), courseId), ids.end());
    }

    bool isEnrolled(int courseId, int studentId) override {
        auto it = courses.studentCourseIds.find(studentId);
        if (it == courses.studentCourseIds.end()) {
            return false;
        }
        return std::find(it->second.begin(), it->second.end(), courseId) != it->second.end();
    }

    std::vector<Course> getCoursesForStudent(int studentId) override {
        return courses.getByStudent(studentId);
    }

    std::vector<std::unique_ptr<User>> getStudentsForCourse(int /* courseId */) override {
        return {};
    }
};

class FakePasswordHasher : public IPasswordHasher {
public:
    std::string hashPassword(const std::string& password) override {
        if (password.size() < 8 || password.size() > 72) {
            throw ValidationException("Password length invalid");
        }
        return "hash:" + password;
    }

    bool verifyPassword(const std::string& password, const std::string& hash) override {
        return hash == "hash:" + password;
    }
};

} // namespace Test
} // namespace Lms
