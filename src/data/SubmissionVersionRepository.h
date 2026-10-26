#pragma once

#include "domain/Entities.h"
#include "data/Database.h"
#include <vector>

namespace Lms {

/**
 * Interface for submission version repository.
 */
class ISubmissionVersionRepository {
public:
    virtual ~ISubmissionVersionRepository() = default;

    /**
     * Get versions for a submission.
     */
    virtual std::vector<SubmissionVersion> getBySubmission(int submissionId) = 0;

    /**
     * Get a specific version.
     */
    virtual std::unique_ptr<SubmissionVersion> get(int submissionId, int versionNo) = 0;

    /**
     * Get the latest version number for a submission.
     */
    virtual int getLatestVersion(int submissionId) = 0;

    /**
     * Save a version.
     */
    virtual void save(const SubmissionVersion& version) = 0;
};

/**
 * SQLite implementation of submission version repository.
 */
class SqliteSubmissionVersionRepository : public ISubmissionVersionRepository {
public:
    explicit SqliteSubmissionVersionRepository(Database& db);

    std::vector<SubmissionVersion> getBySubmission(int submissionId) override;
    std::unique_ptr<SubmissionVersion> get(int submissionId, int versionNo) override;
    int getLatestVersion(int submissionId) override;
    void save(const SubmissionVersion& version) override;

private:
    Database& m_db;

    SubmissionVersion fromRow(sqlite3_stmt* stmt);
};

} // namespace Lms
