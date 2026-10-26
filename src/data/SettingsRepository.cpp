#include "SettingsRepository.h"
#include "domain/LmsException.h"
#include <sstream>

namespace Lms {

SqliteSettingsRepository::SqliteSettingsRepository(Database& db)
    : m_db(db) {}

std::string SqliteSettingsRepository::get(const std::string& key) {
    const char* sql = "SELECT value FROM settings WHERE key = ?;";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        std::string value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        sqlite3_finalize(stmt);
        return value;
    } else if (rc == SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw NotFoundException("Setting not found: " + key);
    } else {
        sqlite3_finalize(stmt);
        throw DatabaseException("Failed to get setting: " + std::string(sqlite3_errmsg(m_db.get())));
    }
}

std::string SqliteSettingsRepository::getWithDefault(const std::string& key, const std::string& defaultValue) {
    if (exists(key)) {
        return get(key);
    }
    return defaultValue;
}

void SqliteSettingsRepository::set(const std::string& key, const std::string& value) {
    const char* sql = "INSERT OR REPLACE INTO settings (key, value) VALUES (?, ?);";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        throw DatabaseException("Failed to set setting: " + std::string(sqlite3_errmsg(m_db.get())));
    }
}

bool SqliteSettingsRepository::exists(const std::string& key) {
    const char* sql = "SELECT 1 FROM settings WHERE key = ?;";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    bool exists = (rc == SQLITE_ROW);
    sqlite3_finalize(stmt);

    return exists;
}

void SqliteSettingsRepository::initializeDefaults() {
    // Password settings
    if (!exists("password_min_length")) {
        set("password_min_length", "8");
    }
    if (!exists("password_max_length")) {
        set("password_max_length", "72");
    }
    if (!exists("bcrypt_cost")) {
        set("bcrypt_cost", "12");
    }
    if (!exists("salt_length")) {
        set("salt_length", "16");
    }
    if (!exists("hash_iterations")) {
        set("hash_iterations", "1000");
    }

    // Submission settings
    if (!exists("max_submission_length")) {
        set("max_submission_length", "20000");
    }

    // Late penalty settings
    if (!exists("late_penalty_percent_per_day")) {
        set("late_penalty_percent_per_day", "10");
    }
    if (!exists("late_penalty_max_days")) {
        set("late_penalty_max_days", "4");
    }
    if (!exists("seconds_per_day")) {
        set("seconds_per_day", "86400");
    }

    // Security settings
    if (!exists("max_login_attempts")) {
        set("max_login_attempts", "5");
    }
    if (!exists("lockout_delay_seconds")) {
        set("lockout_delay_seconds", "5");
    }
}

} // namespace Lms
