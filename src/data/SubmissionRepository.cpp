#include "SubmissionRepository.h"
#include "domain/LmsException.h"

namespace Lms {

SqliteSubmissionRepository::SqliteSubmissionRepository(Database& db)
    : m_db(db) {}

std::unique_ptr<Submission> SqliteSubmissionRepository::getById(int id) {
    const char* sql = "SELECT id, assignment_id, student_id, state, raw_mark, penalty_percent, final_mark, feedback, first_submitted_at "
                      "FROM submission WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, id);

    std::unique_ptr<Submission> submission;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        submission = std::make_unique<Submission>(fromRow(stmt));
    }

    sqlite3_finalize(stmt);

    if (!submission) {
        throw NotFoundException("Submission not found with ID: " + std::to_string(id));
    }

    return submission;
}

std::vector<Submission> SqliteSubmissionRepository::getAll() {
    const char* sql = "SELECT id, assignment_id, student_id, state, raw_mark, penalty_percent, final_mark, feedback, first_submitted_at "
                      "FROM submission;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    std::vector<Submission> submissions;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        submissions.push_back(fromRow(stmt));
    }

    sqlite3_finalize(stmt);
    return submissions;
}

int SqliteSubmissionRepository::save(const Submission& entity) {
    const bool insert = entity.getId() <= 0;
    const char* sql = insert
        ? "INSERT INTO submission (assignment_id, student_id, state, raw_mark, penalty_percent, final_mark, feedback, first_submitted_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?);"
        : "UPDATE submission SET assignment_id = ?, student_id = ?, state = ?, raw_mark = ?, penalty_percent = ?, final_mark = ?, feedback = ?, first_submitted_at = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, entity.getAssignmentId());
    sqlite3_bind_int(stmt, 2, entity.getStudentId());
    std::string stateStr = stateToString(entity.getState());
    sqlite3_bind_text(stmt, 3, stateStr.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 4, entity.getRawMark());
    sqlite3_bind_double(stmt, 5, entity.getPenaltyPercent());
    sqlite3_bind_double(stmt, 6, entity.getFinalMark());
    sqlite3_bind_text(stmt, 7, entity.getFeedback().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, entity.getFirstSubmittedAt().c_str(), -1, SQLITE_STATIC);
    if (!insert) sqlite3_bind_int(stmt, 9, entity.getId());

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw DatabaseException("Failed to save submission: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    int id = insert ? static_cast<int>(sqlite3_last_insert_rowid(m_db.get())) : entity.getId();
    int changed = sqlite3_changes(m_db.get());
    sqlite3_finalize(stmt);
    if (!insert && changed == 0) throw NotFoundException("Submission not found with ID: " + std::to_string(id));
    return id;
}

void SqliteSubmissionRepository::remove(int id) {
    const char* sql = "DELETE FROM submission WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw DatabaseException("Failed to delete submission: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_finalize(stmt);
}

bool SqliteSubmissionRepository::exists(int id) {
    const char* sql = "SELECT COUNT(*) FROM submission WHERE id = ?;";
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

std::vector<Submission> SqliteSubmissionRepository::getByAssignment(int assignmentId) {
    const char* sql = "SELECT id, assignment_id, student_id, state, raw_mark, penalty_percent, final_mark, feedback, first_submitted_at "
                      "FROM submission WHERE assignment_id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, assignmentId);

    std::vector<Submission> submissions;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        submissions.push_back(fromRow(stmt));
    }

    sqlite3_finalize(stmt);
    return submissions;
}

std::vector<Submission> SqliteSubmissionRepository::getByStudent(int studentId) {
    const char* sql = "SELECT id, assignment_id, student_id, state, raw_mark, penalty_percent, final_mark, feedback, first_submitted_at "
                      "FROM submission WHERE student_id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, studentId);

    std::vector<Submission> submissions;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        submissions.push_back(fromRow(stmt));
    }

    sqlite3_finalize(stmt);
    return submissions;
}

std::unique_ptr<Submission> SqliteSubmissionRepository::getByAssignmentAndStudent(int assignmentId, int studentId) {
    const char* sql = "SELECT id, assignment_id, student_id, state, raw_mark, penalty_percent, final_mark, feedback, first_submitted_at "
                      "FROM submission WHERE assignment_id = ? AND student_id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, assignmentId);
    sqlite3_bind_int(stmt, 2, studentId);

    std::unique_ptr<Submission> submission;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        submission = std::make_unique<Submission>(fromRow(stmt));
    }

    sqlite3_finalize(stmt);

    if (!submission) {
        throw NotFoundException("Submission not found for assignment " + std::to_string(assignmentId) +
                              " and student " + std::to_string(studentId));
    }

    return submission;
}

std::vector<Submission> SqliteSubmissionRepository::getByState(SubmissionState state) {
    const char* sql = "SELECT id, assignment_id, student_id, state, raw_mark, penalty_percent, final_mark, feedback, first_submitted_at "
                      "FROM submission WHERE state = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    std::string stateStr = stateToString(state);
    sqlite3_bind_text(stmt, 1, stateStr.c_str(), -1, SQLITE_STATIC);

    std::vector<Submission> submissions;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        submissions.push_back(fromRow(stmt));
    }

    sqlite3_finalize(stmt);
    return submissions;
}

Submission SqliteSubmissionRepository::fromRow(sqlite3_stmt* stmt) {
    int id = sqlite3_column_int(stmt, 0);
    int assignmentId = sqlite3_column_int(stmt, 1);
    int studentId = sqlite3_column_int(stmt, 2);
    std::string stateStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
    double rawMark = sqlite3_column_double(stmt, 4);
    double penaltyPercent = sqlite3_column_double(stmt, 5);
    double finalMark = sqlite3_column_double(stmt, 6);
    std::string feedback = sqlite3_column_type(stmt, 7) == SQLITE_NULL ? "" :
                           reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
    std::string firstSubmittedAt = sqlite3_column_type(stmt, 8) == SQLITE_NULL ? "" :
                                    reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));

    Submission submission(id, assignmentId, studentId, stateFromString(stateStr));
    submission.setRawMark(rawMark);
    submission.setPenaltyPercent(penaltyPercent);
    submission.setFinalMark(finalMark);
    submission.setFeedback(feedback);
    submission.setFirstSubmittedAt(firstSubmittedAt);
    return submission;
}

SubmissionState SqliteSubmissionRepository::stateFromString(const std::string& stateStr) {
    if (stateStr == "SUBMITTED") return SubmissionState::Submitted;
    if (stateStr == "LATE") return SubmissionState::Late;
    if (stateStr == "MARKED") return SubmissionState::Marked;
    if (stateStr == "MODERATED") return SubmissionState::Moderated;
    if (stateStr == "RELEASED") return SubmissionState::Released;
    throw ValidationException("Invalid submission state string: " + stateStr);
}

std::string SqliteSubmissionRepository::stateToString(SubmissionState state) {
    switch (state) {
        case SubmissionState::Submitted: return "SUBMITTED";
        case SubmissionState::Late: return "LATE";
        case SubmissionState::Marked: return "MARKED";
        case SubmissionState::Moderated: return "MODERATED";
        case SubmissionState::Released: return "RELEASED";
        default: throw ValidationException("Invalid submission state");
    }
}

} // namespace Lms
