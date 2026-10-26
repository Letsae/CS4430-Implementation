#include "UserRepository.h"
#include "domain/LmsException.h"
#include <sstream>

namespace Lms {

SqliteUserRepository::SqliteUserRepository(Database& db)
    : m_db(db) {}

std::unique_ptr<User> SqliteUserRepository::getById(int id) {
    const char* sql = "SELECT id, username, role, department_id FROM app_user WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, id);

    std::unique_ptr<User> user;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user = fromRow(stmt);
    }

    sqlite3_finalize(stmt);

    if (!user) {
        throw NotFoundException("User not found with ID: " + std::to_string(id));
    }

    return user;
}

std::vector<std::unique_ptr<User>> SqliteUserRepository::getAll() {
    const char* sql = "SELECT id, username, role, department_id FROM app_user;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    std::vector<std::unique_ptr<User>> users;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        users.push_back(fromRow(stmt));
    }

    sqlite3_finalize(stmt);
    return users;
}

int SqliteUserRepository::save(const User& entity) {
    if (entity.getId() <= 0) throw ValidationException("Use saveWithPassword to create a user");
    int departmentId = 0;
    if (const auto* lecturer = dynamic_cast<const Lecturer*>(&entity)) departmentId = lecturer->getDepartmentId();
    if (const auto* hod = dynamic_cast<const HeadOfDepartment*>(&entity)) departmentId = hod->getDepartmentId();
    const char* sql = "UPDATE app_user SET username = ?, role = ?, department_id = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }
    const std::string role = roleToString(entity.getRole());
    sqlite3_bind_text(stmt, 1, entity.getUsername().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, role.c_str(), -1, SQLITE_TRANSIENT);
    if (departmentId == 0) sqlite3_bind_null(stmt, 3);
    else sqlite3_bind_int(stmt, 3, departmentId);
    sqlite3_bind_int(stmt, 4, entity.getId());
    const int rc = sqlite3_step(stmt);
    const std::string error = sqlite3_errmsg(m_db.get());
    const int changed = sqlite3_changes(m_db.get());
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) throw DatabaseException("Failed to save user: " + error);
    if (changed == 0) throw NotFoundException("User not found with ID: " + std::to_string(entity.getId()));
    return entity.getId();
}

void SqliteUserRepository::remove(int id) {
    const char* sql = "DELETE FROM app_user WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw DatabaseException("Failed to delete user: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_finalize(stmt);
}

bool SqliteUserRepository::exists(int id) {
    const char* sql = "SELECT COUNT(*) FROM app_user WHERE id = ?;";
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

bool SqliteUserRepository::isActive(int id) {
    const char* sql = "SELECT active FROM app_user WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }
    sqlite3_bind_int(stmt, 1, id);
    const int rc = sqlite3_step(stmt);
    const bool active = rc == SQLITE_ROW && sqlite3_column_int(stmt, 0) != 0;
    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        const std::string message = sqlite3_errmsg(m_db.get());
        sqlite3_finalize(stmt);
        throw DatabaseException("Failed to read user status: " + message);
    }
    sqlite3_finalize(stmt);
    return active;
}

std::unique_ptr<User> SqliteUserRepository::getByUsername(const std::string& username) {
    const char* sql = "SELECT id, username, role, department_id FROM app_user WHERE username = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    std::unique_ptr<User> user;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user = fromRow(stmt);
    }

    sqlite3_finalize(stmt);

    if (!user) {
        throw NotFoundException("User not found with username: " + username);
    }

    return user;
}

std::vector<std::unique_ptr<User>> SqliteUserRepository::getByRole(Role role) {
    const char* sql = "SELECT id, username, role, department_id FROM app_user WHERE role = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    std::string roleStr = roleToString(role);
    sqlite3_bind_text(stmt, 1, roleStr.c_str(), -1, SQLITE_STATIC);

    std::vector<std::unique_ptr<User>> users;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        users.push_back(fromRow(stmt));
    }

    sqlite3_finalize(stmt);
    return users;
}

std::vector<std::unique_ptr<User>> SqliteUserRepository::getByDepartment(int departmentId) {
    const char* sql = "SELECT id, username, role, department_id FROM app_user WHERE department_id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int(stmt, 1, departmentId);

    std::vector<std::unique_ptr<User>> users;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        users.push_back(fromRow(stmt));
    }

    sqlite3_finalize(stmt);
    return users;
}

bool SqliteUserRepository::usernameExists(const std::string& username) {
    const char* sql = "SELECT COUNT(*) FROM app_user WHERE username = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = sqlite3_column_int(stmt, 0) > 0;
    }

    sqlite3_finalize(stmt);
    return exists;
}

std::string SqliteUserRepository::getPasswordHash(const std::string& username) {
    const char* sql = "SELECT password_hash FROM app_user WHERE username = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    std::string hash;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* hashPtr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (hashPtr) {
            hash = std::string(hashPtr);
        }
    }

    sqlite3_finalize(stmt);

    if (hash.empty()) {
        throw NotFoundException("User not found with username: " + username);
    }

    return hash;
}

void SqliteUserRepository::setPasswordHash(int userId, const std::string& hash) {
    const char* sql = "UPDATE app_user SET password_hash = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_text(stmt, 1, hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, userId);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw DatabaseException("Failed to update password hash: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_finalize(stmt);
}

int SqliteUserRepository::saveWithPassword(const std::string& username, Role role,
                                             const std::string& passwordHash, int departmentId) {
    const char* sql = "INSERT INTO app_user (username, role, password_hash, department_id, active) VALUES (?, ?, ?, ?, 1);";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    std::string roleStr = roleToString(role);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, roleStr.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, passwordHash.c_str(), -1, SQLITE_STATIC);

    // Bind department_id as NULL if it's 0, otherwise bind the actual ID
    if (departmentId == 0) {
        sqlite3_bind_null(stmt, 4);
    } else {
        sqlite3_bind_int(stmt, 4, departmentId);
    }

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw DatabaseException("Failed to insert user: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    int id = static_cast<int>(sqlite3_last_insert_rowid(m_db.get()));
    sqlite3_finalize(stmt);
    return id;
}

std::unique_ptr<User> SqliteUserRepository::fromRow(sqlite3_stmt* stmt) {
    int id = sqlite3_column_int(stmt, 0);
    std::string username(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
    std::string roleStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
    int departmentId = sqlite3_column_type(stmt, 3) == SQLITE_NULL ? 0 : sqlite3_column_int(stmt, 3);

    Role role = roleFromString(roleStr);
    return UserFactory::create(role, id, username, departmentId);
}

Role SqliteUserRepository::roleFromString(const std::string& roleStr) {
    if (roleStr == "STUDENT") return Role::Student;
    if (roleStr == "LECTURER") return Role::Lecturer;
    if (roleStr == "HOD") return Role::HeadOfDepartment;
    if (roleStr == "ADMIN") return Role::Administrator;
    throw ValidationException("Invalid role string: " + roleStr);
}

std::string SqliteUserRepository::roleToString(Role role) {
    switch (role) {
        case Role::Student: return "STUDENT";
        case Role::Lecturer: return "LECTURER";
        case Role::HeadOfDepartment: return "HOD";
        case Role::Administrator: return "ADMIN";
        default: throw ValidationException("Invalid role");
    }
}

} // namespace Lms
