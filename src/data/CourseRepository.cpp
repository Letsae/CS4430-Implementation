#include "CourseRepository.h"
#include "domain/LmsException.h"

namespace Lms {

SqliteCourseRepository::SqliteCourseRepository(Database& db)
    : m_db(db) {}

std::unique_ptr<Course> SqliteCourseRepository::getById(int id) {
    const char* sql = "SELECT id, code, title, department_id FROM course WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, id);

    std::unique_ptr<Course> course;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        course = std::make_unique<Course>(fromRow(stmt));
    }

    sqlite3_finalize(stmt);

    if (!course) {
        throw NotFoundException("Course not found with ID: " + std::to_string(id));
    }

    return course;
}

std::vector<Course> SqliteCourseRepository::getAll() {
    const char* sql = "SELECT id, code, title, department_id FROM course;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    std::vector<Course> courses;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        courses.push_back(fromRow(stmt));
    }

    sqlite3_finalize(stmt);
    return courses;
}

int SqliteCourseRepository::save(const Course& entity) {
    const bool insert = entity.getId() <= 0;
    const char* sql = insert ? "INSERT INTO course (code, title, department_id) VALUES (?, ?, ?);"
                             : "UPDATE course SET code = ?, title = ?, department_id = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_text(stmt, 1, entity.getCode().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, entity.getTitle().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, entity.getDepartmentId());
    if (!insert) sqlite3_bind_int(stmt, 4, entity.getId());

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        const int error = sqlite3_errcode(m_db.get());
        sqlite3_finalize(stmt);
        if (error == SQLITE_CONSTRAINT || error == SQLITE_CONSTRAINT_UNIQUE) {
            throw ValidationException("Course code already exists or department is invalid");
        }
        throw DatabaseException("Failed to insert course: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    int id = insert ? static_cast<int>(sqlite3_last_insert_rowid(m_db.get())) : entity.getId();
    int changed = sqlite3_changes(m_db.get());
    sqlite3_finalize(stmt);
    if (!insert && changed == 0) throw NotFoundException("Course not found with ID: " + std::to_string(id));
    return id;
}

void SqliteCourseRepository::remove(int id) {
    const char* sql = "DELETE FROM course WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw DatabaseException("Failed to delete course: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_finalize(stmt);
}

bool SqliteCourseRepository::exists(int id) {
    const char* sql = "SELECT COUNT(*) FROM course WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, id);

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = sqlite3_column_int(stmt, 0) > 0;
    }

    sqlite3_finalize(stmt);
    return exists;
}

std::vector<Course> SqliteCourseRepository::getByDepartment(int departmentId) {
    const char* sql = "SELECT id, code, title, department_id FROM course WHERE department_id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, departmentId);

    std::vector<Course> courses;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        courses.push_back(fromRow(stmt));
    }

    sqlite3_finalize(stmt);
    return courses;
}

std::vector<Course> SqliteCourseRepository::getByStudent(int studentId) {
    const char* sql = "SELECT c.id, c.code, c.title, c.department_id FROM course c "
                      "JOIN enrolment e ON c.id = e.course_id WHERE e.student_id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, studentId);

    std::vector<Course> courses;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        courses.push_back(fromRow(stmt));
    }

    sqlite3_finalize(stmt);
    return courses;
}

std::vector<Course> SqliteCourseRepository::getByLecturer(int lecturerId) {
    const char* sql = "SELECT c.id, c.code, c.title, c.department_id FROM course c "
                      "JOIN course_lecturer cl ON c.id = cl.course_id WHERE cl.lecturer_id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, lecturerId);

    std::vector<Course> courses;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        courses.push_back(fromRow(stmt));
    }

    sqlite3_finalize(stmt);
    return courses;
}

std::vector<std::string> SqliteCourseRepository::getLecturerUsernames(int courseId) {
    const char* sql = "SELECT u.username FROM app_user u "
                      "JOIN course_lecturer cl ON u.id = cl.lecturer_id "
                      "WHERE cl.course_id = ? ORDER BY u.username;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, courseId);

    std::vector<std::string> names;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* text = sqlite3_column_text(stmt, 0);
        if (text) {
            names.emplace_back(reinterpret_cast<const char*>(text));
        }
    }

    sqlite3_finalize(stmt);
    return names;
}

void SqliteCourseRepository::assignLecturer(int courseId, int lecturerId) {
    const char* sql = "INSERT INTO course_lecturer (course_id, lecturer_id) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, courseId);
    sqlite3_bind_int(stmt, 2, lecturerId);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        const int error = sqlite3_errcode(m_db.get());
        const std::string message = sqlite3_errmsg(m_db.get());
        sqlite3_finalize(stmt);
        if (error == SQLITE_CONSTRAINT_PRIMARYKEY || error == SQLITE_CONSTRAINT_UNIQUE) {
            throw ValidationException("Lecturer is already assigned to this course");
        }
        throw DatabaseException("Failed to assign lecturer: " + message);
    }

    sqlite3_finalize(stmt);
}

Course SqliteCourseRepository::fromRow(sqlite3_stmt* stmt) {
    int id = sqlite3_column_int(stmt, 0);
    std::string code(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
    std::string title(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
    int departmentId = sqlite3_column_int(stmt, 3);
    return Course(id, code, title, departmentId);
}

} // namespace Lms
