#pragma once
#include "data/MenuRepository.h"
#include "domain/Entities.h"
#include "domain/User.h"
#include <vector>

namespace Lms {

/**
 * Service for building role-based menus.
 */
class MenuService {
public:
    explicit MenuService(IMenuRepository& menuRepo);
    ~MenuService() = default;

    /**
     * Build menu for a user based on their role.
     * @param user The user
     * @return Ordered list of menu items
     */
    std::vector<MenuItem> buildMenu(const User& user);

private:
    IMenuRepository& m_menuRepo;
};

} // namespace Lms
