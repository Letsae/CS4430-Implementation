#pragma once

#include "domain/Entities.h"
#include "data/Database.h"
#include "data/IRepository.h"
#include <vector>

namespace Lms {

class IExtensionRepository {
public:
    virtual ~IExtensionRepository() = default;

    /**
     * Get an extension by ID.
     */
    virtual std::unique_ptr<Extension> getById(int id) = 0;

    /**
     * Get all extensions.
     */
    virtual std::vector<Extension> getAll() = 0;

    /**
     * Save an extension.
     */
    virtual int save(const Extension& entity) = 0;

    /**
     * Get extension for a specific assignment and student.
     */
    virtual std::unique_ptr<Extension> getByAssignmentAndStudent(int assignmentId, int studentId) = 0;

    /**
     * Get all extensions for an assignment.
     */
    virtual std::vector<Extension> getByAssignment(int assignmentId) = 0;
};

class SqliteExtensionRepository : public IExtensionRepository {
public:
    explicit SqliteExtensionRepository(Database& db);

    std::unique_ptr<Extension> getById(int id) override;
    std::vector<Extension> getAll() override;
    int save(const Extension& entity) override;
    std::unique_ptr<Extension> getByAssignmentAndStudent(int assignmentId, int studentId) override;
    std::vector<Extension> getByAssignment(int assignmentId) override;

private:
    Database& m_db;

    Extension fromRow(sqlite3_stmt* stmt);
};

} // namespace Lms
