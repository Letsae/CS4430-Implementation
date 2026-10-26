#include "ExtensionRepository.h"
#include "domain/LmsException.h"

namespace Lms {

SqliteExtensionRepository::SqliteExtensionRepository(Database& db)
    : m_db(db) {}

std::unique_ptr<Extension> SqliteExtensionRepository::getById(int id) {
    const char* sql = "SELECT id, assignment_id, student_id, new_deadline, reason, granted_by "
                      "FROM extension WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, id);

    std::unique_ptr<Extension> extension;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        extension = std::make_unique<Extension>(fromRow(stmt));
    }

    sqlite3_finalize(stmt);

    if (!extension) {
        throw NotFoundException("Extension not found with ID: " + std::to_string(id));
    }

    return extension;
}

std::vector<Extension> SqliteExtensionRepository::getAll() {
    const char* sql = "SELECT id, assignment_id, student_id, new_deadline, reason, granted_by FROM extension;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    std::vector<Extension> extensions;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        extensions.push_back(fromRow(stmt));
    }

    sqlite3_finalize(stmt);
    return extensions;
}

int SqliteExtensionRepository::save(const Extension& entity) {
    const bool insert = entity.getId() <= 0;
    const char* sql = insert
        ? "INSERT INTO extension (assignment_id, student_id, new_deadline, reason, granted_by) VALUES (?, ?, ?, ?, ?);"
        : "UPDATE extension SET assignment_id = ?, student_id = ?, new_deadline = ?, reason = ?, granted_by = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, entity.getAssignmentId());
    sqlite3_bind_int(stmt, 2, entity.getStudentId());
    sqlite3_bind_text(stmt, 3, entity.getNewDeadline().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, entity.getReason().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, entity.getGrantedBy());
    if (!insert) sqlite3_bind_int(stmt, 6, entity.getId());

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw DatabaseException("Failed to save extension: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    int id = insert ? static_cast<int>(sqlite3_last_insert_rowid(m_db.get())) : entity.getId();
    int changed = sqlite3_changes(m_db.get());
    sqlite3_finalize(stmt);
    if (!insert && changed == 0) throw NotFoundException("Extension not found with ID: " + std::to_string(id));
    return id;
}

std::unique_ptr<Extension> SqliteExtensionRepository::getByAssignmentAndStudent(int assignmentId, int studentId) {
    const char* sql = "SELECT id, assignment_id, student_id, new_deadline, reason, granted_by "
                      "FROM extension WHERE assignment_id = ? AND student_id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, assignmentId);
    sqlite3_bind_int(stmt, 2, studentId);

    std::unique_ptr<Extension> extension;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        extension = std::make_unique<Extension>(fromRow(stmt));
    }

    sqlite3_finalize(stmt);
    return extension; // Returns nullptr if not found
}

std::vector<Extension> SqliteExtensionRepository::getByAssignment(int assignmentId) {
    const char* sql = "SELECT id, assignment_id, student_id, new_deadline, reason, granted_by "
                      "FROM extension WHERE assignment_id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, assignmentId);

    std::vector<Extension> extensions;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        extensions.push_back(fromRow(stmt));
    }

    sqlite3_finalize(stmt);
    return extensions;
}

Extension SqliteExtensionRepository::fromRow(sqlite3_stmt* stmt) {
    int id = sqlite3_column_int(stmt, 0);
    int assignmentId = sqlite3_column_int(stmt, 1);
    int studentId = sqlite3_column_int(stmt, 2);
    std::string newDeadline(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
    std::string reason(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
    int grantedBy = sqlite3_column_int(stmt, 5);

    return Extension(id, assignmentId, studentId, newDeadline, reason, grantedBy);
}

} // namespace Lms
