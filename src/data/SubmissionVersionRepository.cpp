#include "SubmissionVersionRepository.h"
#include "domain/LmsException.h"
#include <sstream>

namespace Lms {

SqliteSubmissionVersionRepository::SqliteSubmissionVersionRepository(Database& db)
    : m_db(db) {}

std::vector<SubmissionVersion> SqliteSubmissionVersionRepository::getBySubmission(int submissionId) {
    const char* sql = "SELECT submission_id, version_no, content, submitted_at "
                      "FROM submission_version WHERE submission_id = ? ORDER BY version_no ASC;";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, submissionId);

    std::vector<SubmissionVersion> versions;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        versions.push_back(fromRow(stmt));
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        throw DatabaseException("Failed to get submission versions: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    return versions;
}

std::unique_ptr<SubmissionVersion> SqliteSubmissionVersionRepository::get(int submissionId, int versionNo) {
    const char* sql = "SELECT submission_id, version_no, content, submitted_at "
                      "FROM submission_version WHERE submission_id = ? AND version_no = ?;";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, submissionId);
    sqlite3_bind_int(stmt, 2, versionNo);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        auto version = std::make_unique<SubmissionVersion>(fromRow(stmt));
        sqlite3_finalize(stmt);
        return version;
    } else if (rc == SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw NotFoundException("Submission version not found");
    } else {
        sqlite3_finalize(stmt);
        throw DatabaseException("Failed to get submission version: " + std::string(sqlite3_errmsg(m_db.get())));
    }
}

int SqliteSubmissionVersionRepository::getLatestVersion(int submissionId) {
    const char* sql = "SELECT MAX(version_no) FROM submission_version WHERE submission_id = ?;";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, submissionId);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        int version = sqlite3_column_type(stmt, 0) == SQLITE_NULL ? 0 : sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return version;
    } else {
        sqlite3_finalize(stmt);
        throw DatabaseException("Failed to get latest version: " + std::string(sqlite3_errmsg(m_db.get())));
    }
}

void SqliteSubmissionVersionRepository::save(const SubmissionVersion& version) {
    const char* sql = "INSERT INTO submission_version (submission_id, version_no, content, submitted_at) VALUES (?, ?, ?, ?) "
                      "ON CONFLICT(submission_id, version_no) DO UPDATE SET content = excluded.content, submitted_at = excluded.submitted_at;";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, version.getSubmissionId());
    sqlite3_bind_int(stmt, 2, version.getVersionNo());
    sqlite3_bind_text(stmt, 3, version.getContent().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, version.getSubmittedAt().c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        throw DatabaseException("Failed to save submission version: " + std::string(sqlite3_errmsg(m_db.get())));
    }
}

SubmissionVersion SqliteSubmissionVersionRepository::fromRow(sqlite3_stmt* stmt) {
    SubmissionVersion version;
    version.setSubmissionId(sqlite3_column_int(stmt, 0));
    version.setVersionNo(sqlite3_column_int(stmt, 1));
    version.setContent(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
    version.setSubmittedAt(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
    return version;
}

} // namespace Lms
