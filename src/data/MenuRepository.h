#pragma once
#include "data/Database.h"
#include "domain/Entities.h"
#include "domain/User.h"
#include <string>
#include <vector>

namespace Lms {

/**
 * DTO for menu item.
 */
struct MenuItem {
    int id;
    std::string actionCode;
    std::string label;
    int position;
};

/**
 * Interface for menu repository.
 */
class IMenuRepository {
public:
    virtual ~IMenuRepository() = default;

    /**
     * Get menu items for a specific role, ordered by position.
     * @param role The user role
     * @return Ordered list of menu items
     */
    virtual std::vector<MenuItem> findByRole(Role role) = 0;

    /**
     * Get a menu item by action code.
     * @param actionCode The action code
     * @return The menu item
     * @throws NotFoundException if not found
     */
    virtual MenuItem getByActionCode(const std::string& actionCode) = 0;

    /**
     * Initialize default menu items for all roles.
     */
    virtual void initializeDefaults() = 0;
};

/**
 * SQLite implementation of menu repository.
 */
class SqliteMenuRepository : public IMenuRepository {
public:
    explicit SqliteMenuRepository(Database& db);
    ~SqliteMenuRepository() override = default;

    std::vector<MenuItem> findByRole(Role role) override;
    MenuItem getByActionCode(const std::string& actionCode) override;
    void initializeDefaults() override;

private:
    Database& m_db;
    std::string roleToString(Role role);
};

} // namespace Lms
