#pragma once

#include "domain/Entities.h"
#include "domain/User.h"
#include "data/Database.h"
#include "data/IRepository.h"
#include <string>
#include <vector>

namespace Lms {

/**
 * User-specific repository interface.
 */
class IUserRepository {
public:
    virtual ~IUserRepository() = default;

    /**
     * Get a user by ID.
     * @throws NotFoundException if not found
     */
    virtual std::unique_ptr<User> getById(int id) = 0;

    /**
     * Get all users.
     */
    virtual std::vector<std::unique_ptr<User>> getAll() = 0;

    /**
     * Save a user (insert or update).
     * @return The ID of the saved user.
     */
    virtual int save(const User& entity) = 0;

    /**
     * Delete a user by ID.
     */
    virtual void remove(int id) = 0;

    /**
     * Check if a user exists by ID.
     */
    virtual bool exists(int id) = 0;

    virtual bool isActive(int id) { return exists(id); }

    /**
     * Get a user by username.
     */
    virtual std::unique_ptr<User> getByUsername(const std::string& username) = 0;

    /**
     * Get users by role.
     */
    virtual std::vector<std::unique_ptr<User>> getByRole(Role role) = 0;

    /**
     * Get users by department.
     */
    virtual std::vector<std::unique_ptr<User>> getByDepartment(int departmentId) = 0;

    /**
     * Check if a username exists.
     */
    virtual bool usernameExists(const std::string& username) = 0;

    /**
     * Get password hash for a user.
     */
    virtual std::string getPasswordHash(const std::string& username) = 0;

    /**
     * Set password hash for a user.
     */
    virtual void setPasswordHash(int userId, const std::string& hash) = 0;

    /**
     * Save a user with password hash (for user creation).
     */
    virtual int saveWithPassword(const std::string& username, Role role,
                                  const std::string& passwordHash, int departmentId) = 0;
};

/**
 * SQLite implementation of IUserRepository.
 */
class SqliteUserRepository : public IUserRepository {
public:
    explicit SqliteUserRepository(Database& db);

    std::unique_ptr<User> getById(int id) override;
    std::vector<std::unique_ptr<User>> getAll() override;
    int save(const User& entity) override;
    void remove(int id) override;
    bool exists(int id) override;
    bool isActive(int id) override;

    std::unique_ptr<User> getByUsername(const std::string& username) override;
    std::vector<std::unique_ptr<User>> getByRole(Role role) override;
    std::vector<std::unique_ptr<User>> getByDepartment(int departmentId) override;
    bool usernameExists(const std::string& username) override;
    std::string getPasswordHash(const std::string& username) override;
    void setPasswordHash(int userId, const std::string& hash) override;
    int saveWithPassword(const std::string& username, Role role,
                          const std::string& passwordHash, int departmentId) override;

private:
    Database& m_db;

    std::unique_ptr<User> fromRow(sqlite3_stmt* stmt);
    Role roleFromString(const std::string& roleStr);
    std::string roleToString(Role role);
};

} // namespace Lms
