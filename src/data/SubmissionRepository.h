#pragma once

#include "domain/Entities.h"
#include "data/Database.h"
#include "data/IRepository.h"
#include <vector>

namespace Lms {

class ISubmissionRepository : public IRepository<Submission> {
public:
    virtual ~ISubmissionRepository() = default;

    /**
     * Get submissions by assignment.
     */
    virtual std::vector<Submission> getByAssignment(int assignmentId) = 0;

    /**
     * Get submissions by student.
     */
    virtual std::vector<Submission> getByStudent(int studentId) = 0;

    /**
     * Get submission by assignment and student.
     */
    virtual std::unique_ptr<Submission> getByAssignmentAndStudent(int assignmentId, int studentId) = 0;

    /**
     * Get submissions in a specific state.
     */
    virtual std::vector<Submission> getByState(SubmissionState state) = 0;
};

class SqliteSubmissionRepository : public ISubmissionRepository {
public:
    explicit SqliteSubmissionRepository(Database& db);

    std::unique_ptr<Submission> getById(int id) override;
    std::vector<Submission> getAll() override;
    int save(const Submission& entity) override;
    void remove(int id) override;
    bool exists(int id) override;

    std::vector<Submission> getByAssignment(int assignmentId) override;
    std::vector<Submission> getByStudent(int studentId) override;
    std::unique_ptr<Submission> getByAssignmentAndStudent(int assignmentId, int studentId) override;
    std::vector<Submission> getByState(SubmissionState state) override;

private:
    Database& m_db;

    Submission fromRow(sqlite3_stmt* stmt);
    SubmissionState stateFromString(const std::string& stateStr);
    std::string stateToString(SubmissionState state);
};

} // namespace Lms
