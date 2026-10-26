#include "MarkAuditRepository.h"
#include "domain/LmsException.h"

namespace Lms {

SqliteMarkAuditRepository::SqliteMarkAuditRepository(Database& db)
    : m_db(db) {}

void SqliteMarkAuditRepository::add(const MarkAuditEntry& entry) {
    const char* sql = "INSERT INTO mark_audit (submission_id, changed_by, changed_at, old_value, new_value, reason) "
                      "VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, entry.getSubmissionId());
    sqlite3_bind_int(stmt, 2, entry.getChangedBy());
    sqlite3_bind_text(stmt, 3, entry.getChangedAt().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 4, entry.getOldValue());
    sqlite3_bind_double(stmt, 5, entry.getNewValue());
    sqlite3_bind_text(stmt, 6, entry.getReason().c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw DatabaseException("Failed to insert audit entry: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_finalize(stmt);
}

std::vector<MarkAuditEntry> SqliteMarkAuditRepository::getBySubmission(int submissionId) {
    const char* sql = "SELECT id, submission_id, changed_by, changed_at, old_value, new_value, reason "
                      "FROM mark_audit WHERE submission_id = ? ORDER BY changed_at;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, submissionId);

    std::vector<MarkAuditEntry> entries;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        entries.push_back(fromRow(stmt));
    }

    sqlite3_finalize(stmt);
    return entries;
}

std::vector<MarkAuditEntry> SqliteMarkAuditRepository::getAll() {
    const char* sql = "SELECT id, submission_id, changed_by, changed_at, old_value, new_value, reason "
                      "FROM mark_audit ORDER BY changed_at;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    std::vector<MarkAuditEntry> entries;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        entries.push_back(fromRow(stmt));
    }

    sqlite3_finalize(stmt);
    return entries;
}

MarkAuditEntry SqliteMarkAuditRepository::fromRow(sqlite3_stmt* stmt) {
    int id = sqlite3_column_int(stmt, 0);
    int submissionId = sqlite3_column_int(stmt, 1);
    int changedBy = sqlite3_column_int(stmt, 2);
    std::string changedAt(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
    double oldValue = sqlite3_column_double(stmt, 4);
    double newValue = sqlite3_column_double(stmt, 5);
    std::string reason(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));

    return MarkAuditEntry(id, submissionId, changedBy, changedAt, oldValue, newValue, reason);
}

} // namespace Lms
