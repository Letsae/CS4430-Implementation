#include "AssignmentRepository.h"
#include "domain/LmsException.h"

namespace Lms {

SqliteAssignmentRepository::SqliteAssignmentRepository(Database& db)
    : m_db(db) {}

std::unique_ptr<Assignment> SqliteAssignmentRepository::getById(int id) {
    const char* sql = "SELECT id, course_id, title, deadline, max_mark, created_by FROM assignment WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, id);

    std::unique_ptr<Assignment> assignment;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        assignment = std::make_unique<Assignment>(fromRow(stmt));
    }

    sqlite3_finalize(stmt);

    if (!assignment) {
        throw NotFoundException("Assignment not found with ID: " + std::to_string(id));
    }

    return assignment;
}

std::vector<Assignment> SqliteAssignmentRepository::getAll() {
    const char* sql = "SELECT id, course_id, title, deadline, max_mark, created_by FROM assignment;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    std::vector<Assignment> assignments;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        assignments.push_back(fromRow(stmt));
    }

    sqlite3_finalize(stmt);
    return assignments;
}

int SqliteAssignmentRepository::save(const Assignment& entity) {
    const bool insert = entity.getId() <= 0;
    const char* sql = insert ? "INSERT INTO assignment (course_id, title, deadline, max_mark, created_by) VALUES (?, ?, ?, ?, ?);"
                             : "UPDATE assignment SET course_id = ?, title = ?, deadline = ?, max_mark = ?, created_by = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, entity.getCourseId());
    sqlite3_bind_text(stmt, 2, entity.getTitle().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, entity.getDeadline().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 4, entity.getMaxMark());
    sqlite3_bind_int(stmt, 5, entity.getCreatedBy());
    if (!insert) sqlite3_bind_int(stmt, 6, entity.getId());

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::string error = sqlite3_errmsg(m_db.get());
        sqlite3_finalize(stmt);
        throw DatabaseException("Failed to save assignment: " + error);
    }

    int id = insert ? static_cast<int>(sqlite3_last_insert_rowid(m_db.get())) : entity.getId();
    int changed = sqlite3_changes(m_db.get());
    sqlite3_finalize(stmt);
    if (!insert && changed == 0) throw NotFoundException("Assignment not found with ID: " + std::to_string(id));
    return id;
}

void SqliteAssignmentRepository::remove(int id) {
    const char* sql = "DELETE FROM assignment WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw DatabaseException("Failed to delete assignment: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_finalize(stmt);
}

bool SqliteAssignmentRepository::exists(int id) {
    const char* sql = "SELECT COUNT(*) FROM assignment WHERE id = ?;";
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

std::vector<Assignment> SqliteAssignmentRepository::getByCourse(int courseId) {
    const char* sql = "SELECT id, course_id, title, deadline, max_mark, created_by FROM assignment WHERE course_id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, courseId);

    std::vector<Assignment> assignments;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        assignments.push_back(fromRow(stmt));
    }

    sqlite3_finalize(stmt);
    return assignments;
}

Assignment SqliteAssignmentRepository::fromRow(sqlite3_stmt* stmt) {
    int id = sqlite3_column_int(stmt, 0);
    int courseId = sqlite3_column_int(stmt, 1);
    std::string title(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
    std::string deadline(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
    double maxMark = sqlite3_column_double(stmt, 4);
    int createdBy = sqlite3_column_int(stmt, 5);
    return Assignment(id, courseId, title, deadline, maxMark, createdBy);
}

} // namespace Lms
