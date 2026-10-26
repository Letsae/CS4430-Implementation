#include "DepartmentRepository.h"
#include "domain/LmsException.h"

namespace Lms {

SqliteDepartmentRepository::SqliteDepartmentRepository(Database& db)
    : m_db(db) {}

std::unique_ptr<Department> SqliteDepartmentRepository::getById(int id) {
    const char* sql = "SELECT id, name FROM department WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, id);

    std::unique_ptr<Department> department;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        department = std::make_unique<Department>(fromRow(stmt));
    }

    sqlite3_finalize(stmt);

    if (!department) {
        throw NotFoundException("Department not found with ID: " + std::to_string(id));
    }

    return department;
}

std::vector<Department> SqliteDepartmentRepository::getAll() {
    const char* sql = "SELECT id, name FROM department;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    std::vector<Department> departments;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        departments.push_back(fromRow(stmt));
    }

    sqlite3_finalize(stmt);
    return departments;
}

int SqliteDepartmentRepository::save(const Department& entity) {
    const bool insert = entity.getId() <= 0;
    const char* sql = insert ? "INSERT INTO department (name) VALUES (?);"
                             : "UPDATE department SET name = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_text(stmt, 1, entity.getName().c_str(), -1, SQLITE_STATIC);
    if (!insert) sqlite3_bind_int(stmt, 2, entity.getId());

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        const int error = sqlite3_errcode(m_db.get());
        sqlite3_finalize(stmt);
        if (error == SQLITE_CONSTRAINT || error == SQLITE_CONSTRAINT_UNIQUE) {
            throw ValidationException("Department name already exists");
        }
        throw DatabaseException("Failed to insert department: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    int id = insert ? static_cast<int>(sqlite3_last_insert_rowid(m_db.get())) : entity.getId();
    int changed = sqlite3_changes(m_db.get());
    sqlite3_finalize(stmt);
    if (!insert && changed == 0) throw NotFoundException("Department not found with ID: " + std::to_string(id));
    return id;
}

void SqliteDepartmentRepository::remove(int id) {
    const char* sql = "DELETE FROM department WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw DatabaseException("Failed to delete department: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_finalize(stmt);
}

bool SqliteDepartmentRepository::exists(int id) {
    const char* sql = "SELECT COUNT(*) FROM department WHERE id = ?;";
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

Department SqliteDepartmentRepository::fromRow(sqlite3_stmt* stmt) {
    int id = sqlite3_column_int(stmt, 0);
    std::string name(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
    return Department(id, name);
}

} // namespace Lms
