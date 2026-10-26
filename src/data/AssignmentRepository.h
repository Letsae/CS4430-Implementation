#pragma once

#include "domain/Entities.h"
#include "data/Database.h"
#include "data/IRepository.h"
#include <vector>

namespace Lms {

class IAssignmentRepository : public IRepository<Assignment> {
public:
    virtual ~IAssignmentRepository() = default;

    /**
     * Get assignments by course.
     */
    virtual std::vector<Assignment> getByCourse(int courseId) = 0;
};

class SqliteAssignmentRepository : public IAssignmentRepository {
public:
    explicit SqliteAssignmentRepository(Database& db);

    std::unique_ptr<Assignment> getById(int id) override;
    std::vector<Assignment> getAll() override;
    int save(const Assignment& entity) override;
    void remove(int id) override;
    bool exists(int id) override;

    std::vector<Assignment> getByCourse(int courseId) override;

private:
    Database& m_db;

    Assignment fromRow(sqlite3_stmt* stmt);
};

} // namespace Lms
