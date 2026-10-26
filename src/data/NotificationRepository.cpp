#include "NotificationRepository.h"
#include "domain/LmsException.h"

namespace Lms {

SqliteNotificationRepository::SqliteNotificationRepository(Database& db) : m_db(db) {}

std::unique_ptr<Notification> SqliteNotificationRepository::getById(int id) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id,user_id,message,created_at,is_read FROM notification WHERE id=?;";
    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw DatabaseException("Failed to prepare notification query: " + std::string(sqlite3_errmsg(m_db.get())));
    sqlite3_bind_int(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    std::unique_ptr<Notification> result;
    if (rc == SQLITE_ROW) {
        result = std::make_unique<Notification>(sqlite3_column_int(stmt, 0), sqlite3_column_int(stmt, 1),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)), sqlite3_column_int(stmt, 4) != 0);
    }
    sqlite3_finalize(stmt);
    if (rc != SQLITE_ROW && rc != SQLITE_DONE)
        throw DatabaseException("Failed to read notification: " + std::string(sqlite3_errmsg(m_db.get())));
    if (!result) throw NotFoundException("Notification not found with ID: " + std::to_string(id));
    return result;
}

std::vector<Notification> SqliteNotificationRepository::getAll() {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id,user_id,message,created_at,is_read FROM notification ORDER BY id;";
    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw DatabaseException("Failed to prepare notification query: " + std::string(sqlite3_errmsg(m_db.get())));
    std::vector<Notification> rows;
    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
        rows.emplace_back(sqlite3_column_int(stmt, 0), sqlite3_column_int(stmt, 1),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)), sqlite3_column_int(stmt, 4) != 0);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) throw DatabaseException("Failed to read notifications: " + std::string(sqlite3_errmsg(m_db.get())));
    return rows;
}

int SqliteNotificationRepository::save(const Notification& n) {
    sqlite3_stmt* stmt = nullptr;
    const bool insert = n.getId() <= 0;
    const char* sql = insert ? "INSERT INTO notification(user_id,message,created_at,is_read) VALUES(?,?,?,?);"
                             : "UPDATE notification SET user_id=?, message=?, created_at=?, is_read=? WHERE id=?;";
    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw DatabaseException("Failed to prepare notification insert: " + std::string(sqlite3_errmsg(m_db.get())));
    sqlite3_bind_int(stmt, 1, n.getUserId());
    sqlite3_bind_text(stmt, 2, n.getMessage().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, n.getCreatedAt().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, n.isRead() ? 1 : 0);
    if (!insert) sqlite3_bind_int(stmt, 5, n.getId());
    int rc = sqlite3_step(stmt);
    const std::string error = sqlite3_errmsg(m_db.get());
    const int changed = sqlite3_changes(m_db.get());
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) throw DatabaseException("Failed to save notification: " + error);
    const int id = insert ? static_cast<int>(sqlite3_last_insert_rowid(m_db.get())) : n.getId();
    if (!insert && changed == 0) throw NotFoundException("Notification not found with ID: " + std::to_string(id));
    return id;
}

void SqliteNotificationRepository::remove(int id) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db.get(), "DELETE FROM notification WHERE id=?;", -1, &stmt, nullptr) != SQLITE_OK)
        throw DatabaseException("Failed to prepare notification delete: " + std::string(sqlite3_errmsg(m_db.get())));
    sqlite3_bind_int(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) throw DatabaseException("Failed to delete notification: " + std::string(sqlite3_errmsg(m_db.get())));
}

bool SqliteNotificationRepository::exists(int id) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db.get(), "SELECT 1 FROM notification WHERE id=?;", -1, &stmt, nullptr) != SQLITE_OK)
        throw DatabaseException("Failed to prepare notification query: " + std::string(sqlite3_errmsg(m_db.get())));
    sqlite3_bind_int(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_ROW && rc != SQLITE_DONE) throw DatabaseException("Failed to read notification: " + std::string(sqlite3_errmsg(m_db.get())));
    return rc == SQLITE_ROW;
}

} // namespace Lms
