#include "MenuService.h"
#include "domain/User.h"

namespace Lms {

MenuService::MenuService(IMenuRepository& menuRepo)
    : m_menuRepo(menuRepo) {}

std::vector<MenuItem> MenuService::buildMenu(const User& user) {
    return m_menuRepo.findByRole(user.getRole());
}

} // namespace Lms
