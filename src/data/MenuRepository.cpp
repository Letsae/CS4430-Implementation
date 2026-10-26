#include "MenuRepository.h"
#include "domain/LmsException.h"
#include <sstream>

namespace Lms {

SqliteMenuRepository::SqliteMenuRepository(Database& db)
    : m_db(db) {}

std::string SqliteMenuRepository::roleToString(Role role) {
    switch (role) {
        case Role::Student: return "STUDENT";
        case Role::Lecturer: return "LECTURER";
        case Role::HeadOfDepartment: return "HOD";
        case Role::Administrator: return "ADMIN";
        default: throw ValidationException("Invalid role");
    }
}

std::vector<MenuItem> SqliteMenuRepository::findByRole(Role role) {
    std::string roleStr = roleToString(role);
    const char* sql = "SELECT mi.id, mi.action_code, mi.label, rmi.position "
                      "FROM menu_item mi "
                      "JOIN role_menu_item rmi ON mi.id = rmi.menu_item_id "
                      "WHERE rmi.role = ? "
                      "ORDER BY rmi.position ASC;";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_text(stmt, 1, roleStr.c_str(), -1, SQLITE_STATIC);

    std::vector<MenuItem> items;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        MenuItem item;
        item.id = sqlite3_column_int(stmt, 0);
        item.actionCode = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        item.label = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        item.position = sqlite3_column_int(stmt, 3);
        items.push_back(item);
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        throw DatabaseException("Failed to get menu items: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    return items;
}

MenuItem SqliteMenuRepository::getByActionCode(const std::string& actionCode) {
    const char* sql = "SELECT id, action_code, label, 0 FROM menu_item WHERE action_code = ?;";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_text(stmt, 1, actionCode.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        MenuItem item;
        item.id = sqlite3_column_int(stmt, 0);
        item.actionCode = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        item.label = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        item.position = 0;
        sqlite3_finalize(stmt);
        return item;
    } else if (rc == SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw NotFoundException("Menu item not found: " + actionCode);
    } else {
        sqlite3_finalize(stmt);
        throw DatabaseException("Failed to get menu item: " + std::string(sqlite3_errmsg(m_db.get())));
    }
}

void SqliteMenuRepository::initializeDefaults() {
    // Insert menu items
    const char* insertItemSql = "INSERT OR IGNORE INTO menu_item (action_code, label) VALUES (?, ?);";
    const char* insertRoleSql = "INSERT OR IGNORE INTO role_menu_item (role, menu_item_id, position) VALUES (?, ?, ?);";

    // Define all menu items
    struct MenuDef {
        const char* actionCode;
        const char* label;
    };

    MenuDef items[] = {
        {"VIEW_COURSES", "View My Courses"},
        {"VIEW_SUBMISSIONS", "View My Submissions"},
        {"SUBMIT_ASSIGNMENT", "Submit Assignment"},
        {"CHANGE_PASSWORD", "Change Password"},
        {"CREATE_ASSIGNMENT", "Create Assignment"},
        {"VIEW_ALL_SUBMISSIONS", "View Submissions"},
        {"MARK_SUBMISSION", "Mark Submission"},
        {"GRANT_EXTENSION", "Grant Extension"},
        {"VIEW_DEPT_COURSES", "View Department Courses"},
        {"VIEW_MARKED_SUBMISSIONS", "View Marked Submissions"},
        {"MODERATE_SUBMISSION", "Moderate Submission"},
        {"RELEASE_MARKS", "Release Marks"},
        {"CREATE_USER", "Create User"},
        {"CREATE_DEPARTMENT", "Create Department"},
        {"CREATE_COURSE", "Create Course"},
        {"ASSIGN_LECTURER", "Assign Lecturer to Course"},
        {"ENROLL_STUDENT", "Enroll Student in Course"}
    };

    // Insert menu items
    for (const auto& item : items) {
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(m_db.get(), insertItemSql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
        }
        sqlite3_bind_text(stmt, 1, item.actionCode, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, item.label, -1, SQLITE_STATIC);
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (rc != SQLITE_DONE) {
            throw DatabaseException("Failed to insert menu item: " + std::string(sqlite3_errmsg(m_db.get())));
        }
    }

    // Map roles to menu items
    struct RoleMapping {
        const char* role;
        const char* actionCode;
        int position;
    };

    RoleMapping mappings[] = {
        // Student menu
        {"STUDENT", "VIEW_COURSES", 1},
        {"STUDENT", "VIEW_SUBMISSIONS", 2},
        {"STUDENT", "SUBMIT_ASSIGNMENT", 3},
        {"STUDENT", "CHANGE_PASSWORD", 4},

        // Lecturer menu
        {"LECTURER", "VIEW_COURSES", 1},
        {"LECTURER", "CREATE_ASSIGNMENT", 2},
        {"LECTURER", "VIEW_ALL_SUBMISSIONS", 3},
        {"LECTURER", "MARK_SUBMISSION", 4},
        {"LECTURER", "GRANT_EXTENSION", 5},
        {"LECTURER", "CHANGE_PASSWORD", 6},

        // HOD menu
        {"HOD", "VIEW_DEPT_COURSES", 1},
        {"HOD", "VIEW_MARKED_SUBMISSIONS", 2},
        {"HOD", "MODERATE_SUBMISSION", 3},
        {"HOD", "RELEASE_MARKS", 4},
        {"HOD", "CHANGE_PASSWORD", 5},

        // Admin menu
        {"ADMIN", "CREATE_USER", 1},
        {"ADMIN", "CREATE_DEPARTMENT", 2},
        {"ADMIN", "CREATE_COURSE", 3},
        {"ADMIN", "ASSIGN_LECTURER", 4},
        {"ADMIN", "ENROLL_STUDENT", 5},
        {"ADMIN", "CHANGE_PASSWORD", 6}
    };

    // Insert role mappings
    for (const auto& mapping : mappings) {
        // Get menu item ID
        MenuItem item = getByActionCode(mapping.actionCode);

        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(m_db.get(), insertRoleSql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            throw DatabaseException("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db.get())));
        }
        sqlite3_bind_text(stmt, 1, mapping.role, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, item.id);
        sqlite3_bind_int(stmt, 3, mapping.position);
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (rc != SQLITE_DONE) {
            throw DatabaseException("Failed to insert role mapping: " + std::string(sqlite3_errmsg(m_db.get())));
        }
    }
}

} // namespace Lms
