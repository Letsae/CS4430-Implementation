#pragma once

#include <string>
#include <sqlite3.h>
#include <memory>

namespace Lms {

/**
 * RAII wrapper for SQLite database connection.
 */
class Database {
public:
    /**
     * Open a database connection.
     * @param path Database file path, or ":memory:" for in-memory database.
     */
    explicit Database(const std::string& path);
    ~Database();

    // Prevent copying
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    /**
     * Get the raw SQLite3 pointer.
     */
    sqlite3* get() const { return m_db; }

    /**
     * Execute a SQL statement (for schema setup, etc.).
     */
    void execute(const std::string& sql);

    /**
     * Begin a transaction.
     */
    void beginTransaction();

    /**
     * Commit the current transaction.
     */
    void commit();

    /**
     * Rollback the current transaction.
     */
    void rollback();

    /**
     * Check if a transaction is active.
     */
    bool isInTransaction() const { return m_inTransaction; }

private:
    sqlite3* m_db = nullptr;
    bool m_inTransaction = false;
};

/**
 * RAII transaction guard that commits on success and rolls back on exception.
 */
class TransactionGuard {
public:
    explicit TransactionGuard(Database& db);
    ~TransactionGuard();

    // Prevent copying
    TransactionGuard(const TransactionGuard&) = delete;
    TransactionGuard& operator=(const TransactionGuard&) = delete;

    /**
     * Commit the transaction early and disable rollback in destructor.
     */
    void commit();

private:
    Database& m_db;
    bool m_committed = false;
};

} // namespace Lms
