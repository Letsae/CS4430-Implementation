#pragma once

#include "domain/Entities.h"
#include "data/Database.h"
#include "data/IRepository.h"
#include <vector>

namespace Lms {

class IDepartmentRepository : public IRepository<Department> {
public:
    virtual ~IDepartmentRepository() = default;
};

class SqliteDepartmentRepository : public IDepartmentRepository {
public:
    explicit SqliteDepartmentRepository(Database& db);

    std::unique_ptr<Department> getById(int id) override;
    std::vector<Department> getAll() override;
    int save(const Department& entity) override;
    void remove(int id) override;
    bool exists(int id) override;

private:
    Database& m_db;

    Department fromRow(sqlite3_stmt* stmt);
};

} // namespace Lms
