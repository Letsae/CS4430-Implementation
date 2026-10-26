#pragma once

#include "domain/Entities.h"
#include "data/Database.h"
#include "data/IRepository.h"
#include <string>
#include <vector>

namespace Lms {

class ICourseRepository : public IRepository<Course> {
public:
    virtual ~ICourseRepository() = default;

    /**
     * Get courses by department.
     */
    virtual std::vector<Course> getByDepartment(int departmentId) = 0;

    /**
     * Get courses for a student (via enrolment).
     */
    virtual std::vector<Course> getByStudent(int studentId) = 0;

    /**
     * Get courses for a lecturer.
     */
    virtual std::vector<Course> getByLecturer(int lecturerId) = 0;

    /**
     * Usernames of lecturers assigned to a course (via course_lecturer).
     */
    virtual std::vector<std::string> getLecturerUsernames(int courseId) = 0;

    /**
     * Assign a lecturer to a course.
     */
    virtual void assignLecturer(int courseId, int lecturerId) = 0;
};

class SqliteCourseRepository : public ICourseRepository {
public:
    explicit SqliteCourseRepository(Database& db);

    std::unique_ptr<Course> getById(int id) override;
    std::vector<Course> getAll() override;
    int save(const Course& entity) override;
    void remove(int id) override;
    bool exists(int id) override;

    std::vector<Course> getByDepartment(int departmentId) override;
    std::vector<Course> getByStudent(int studentId) override;
    std::vector<Course> getByLecturer(int lecturerId) override;
    std::vector<std::string> getLecturerUsernames(int courseId) override;
    void assignLecturer(int courseId, int lecturerId) override;

private:
    Database& m_db;

    Course fromRow(sqlite3_stmt* stmt);
};

} // namespace Lms
