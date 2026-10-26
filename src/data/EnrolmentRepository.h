#pragma once

#include "domain/Entities.h"
#include "domain/User.h"
#include "data/Database.h"
#include "data/IRepository.h"
#include "data/CourseRepository.h"
#include "data/UserRepository.h"
#include <vector>

namespace Lms {

class IEnrolmentRepository {
public:
    virtual ~IEnrolmentRepository() = default;

    /**
     * Enroll a student in a course.
     */
    virtual void enroll(int courseId, int studentId) = 0;

    /**
     * Unenroll a student from a course.
     */
    virtual void unenroll(int courseId, int studentId) = 0;

    /**
     * Check if a student is enrolled in a course.
     */
    virtual bool isEnrolled(int courseId, int studentId) = 0;

    /**
     * Get all courses for a student.
     */
    virtual std::vector<Course> getCoursesForStudent(int studentId) = 0;

    /**
     * Get all students enrolled in a course.
     */
    virtual std::vector<std::unique_ptr<User>> getStudentsForCourse(int courseId) = 0;
};

class SqliteEnrolmentRepository : public IEnrolmentRepository {
public:
    explicit SqliteEnrolmentRepository(Database& db, ICourseRepository& courseRepo, IUserRepository& userRepo);

    void enroll(int courseId, int studentId) override;
    void unenroll(int courseId, int studentId) override;
    bool isEnrolled(int courseId, int studentId) override;
    std::vector<Course> getCoursesForStudent(int studentId) override;
    std::vector<std::unique_ptr<User>> getStudentsForCourse(int courseId) override;

private:
    Database& m_db;
    ICourseRepository& m_courseRepo;
    IUserRepository& m_userRepo;
};

} // namespace Lms
