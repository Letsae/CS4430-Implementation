#pragma once

#include "domain/Entities.h"
#include "data/Database.h"
#include <vector>

namespace Lms {

class IMarkAuditRepository {
public:
    virtual ~IMarkAuditRepository() = default;

    /**
     * Add an audit entry (append-only, no update/delete).
     */
    virtual void add(const MarkAuditEntry& entry) = 0;

    /**
     * Get audit entries for a submission.
     */
    virtual std::vector<MarkAuditEntry> getBySubmission(int submissionId) = 0;

    /**
     * Get all audit entries.
     */
    virtual std::vector<MarkAuditEntry> getAll() = 0;
};

class SqliteMarkAuditRepository : public IMarkAuditRepository {
public:
    explicit SqliteMarkAuditRepository(Database& db);

    void add(const MarkAuditEntry& entry) override;
    std::vector<MarkAuditEntry> getBySubmission(int submissionId) override;
    std::vector<MarkAuditEntry> getAll() override;

private:
    Database& m_db;

    MarkAuditEntry fromRow(sqlite3_stmt* stmt);
};

} // namespace Lms
