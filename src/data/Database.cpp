#include "Database.h"
#include "domain/LmsException.h"
#include <iostream>

namespace Lms {

Database::Database(const std::string& path) {
    int result = sqlite3_open(path.c_str(), &m_db);
    if (result != SQLITE_OK) {
        throw DatabaseException("Failed to open database: " + std::string(sqlite3_errmsg(m_db)));
    }

    // Enable foreign keys
    execute("PRAGMA foreign_keys = ON;");

    // Enable WAL mode
    execute("PRAGMA journal_mode = WAL;");
}

Database::~Database() {
    if (m_db) {
        sqlite3_close(m_db);
    }
}

void Database::execute(const std::string& sql) {
    char* errorMsg = nullptr;
    int result = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errorMsg);
    if (result != SQLITE_OK) {
        std::string error = errorMsg ? errorMsg : "Unknown error";
        sqlite3_free(errorMsg);
        throw DatabaseException("SQL execution failed: " + error);
    }
}

void Database::beginTransaction() {
    if (m_inTransaction) {
        throw DatabaseException("Transaction already in progress");
    }
    execute("BEGIN TRANSACTION;");
    m_inTransaction = true;
}

void Database::commit() {
    if (!m_inTransaction) {
        throw DatabaseException("No transaction in progress");
    }
    execute("COMMIT;");
    m_inTransaction = false;
}

void Database::rollback() {
    if (!m_inTransaction) {
        return; // Nothing to rollback
    }
    execute("ROLLBACK;");
    m_inTransaction = false;
}

TransactionGuard::TransactionGuard(Database& db)
    : m_db(db) {
    m_db.beginTransaction();
}

TransactionGuard::~TransactionGuard() {
    if (!m_committed && m_db.isInTransaction()) {
        try {
            m_db.rollback();
        } catch (...) {
            // Suppress exceptions in destructor
        }
    }
}

void TransactionGuard::commit() {
    if (!m_committed) {
        m_db.commit();
        m_committed = true;
    }
}

} // namespace Lms
