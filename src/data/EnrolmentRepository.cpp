#include "EnrolmentRepository.h"
#include "domain/LmsException.h"
#include "domain/User.h"
#include "util/Clock.h"

namespace Lms {

SqliteEnrolmentRepository::SqliteEnrolmentRepository(Database& db, ICourseRepository& courseRepo, IUserRepository& userRepo)
    : m_db(db), m_courseRepo(courseRepo), m_userRepo(userRepo) {}

void SqliteEnrolmentRepository::enroll(int courseId, int studentId) {
    const char* sql = "INSERT INTO enrolment (course_id, student_id, enrolled_on) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    SystemClock clock;
    std::string now = clock.now();

    sqlite3_bind_int(stmt, 1, courseId);
    sqlite3_bind_int(stmt, 2, studentId);
    sqlite3_bind_text(stmt, 3, now.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw DatabaseException("Failed to enroll student: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_finalize(stmt);
}

void SqliteEnrolmentRepository::unenroll(int courseId, int studentId) {
    const char* sql = "DELETE FROM enrolment WHERE course_id = ? AND student_id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, courseId);
    sqlite3_bind_int(stmt, 2, studentId);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw DatabaseException("Failed to unenroll student: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_finalize(stmt);
}

bool SqliteEnrolmentRepository::isEnrolled(int courseId, int studentId) {
    const char* sql = "SELECT COUNT(*) FROM enrolment WHERE course_id = ? AND student_id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, courseId);
    sqlite3_bind_int(stmt, 2, studentId);

    bool enrolled = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        enrolled = sqlite3_column_int(stmt, 0) > 0;
    }

    sqlite3_finalize(stmt);
    return enrolled;
}

std::vector<Course> SqliteEnrolmentRepository::getCoursesForStudent(int studentId) {
    return m_courseRepo.getByStudent(studentId);
}

std::vector<std::unique_ptr<User>> SqliteEnrolmentRepository::getStudentsForCourse(int courseId) {
    const char* sql = "SELECT u.id, u.username, u.role, u.department_id "
                      "FROM app_user u "
                      "JOIN enrolment e ON u.id = e.student_id "
                      "WHERE e.course_id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, courseId);

    std::vector<std::unique_ptr<User>> students;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string username(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        std::string roleStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        int departmentId = sqlite3_column_type(stmt, 3) == SQLITE_NULL ? 0 : sqlite3_column_int(stmt, 3);

        Role role;
        if (roleStr == "STUDENT") role = Role::Student;
        else if (roleStr == "LECTURER") role = Role::Lecturer;
        else if (roleStr == "HOD") role = Role::HeadOfDepartment;
        else if (roleStr == "ADMIN") role = Role::Administrator;
        else throw ValidationException("Invalid role");

        students.push_back(UserFactory::create(role, id, username, departmentId));
    }

    sqlite3_finalize(stmt);
    return students;
}

} // namespace Lms
