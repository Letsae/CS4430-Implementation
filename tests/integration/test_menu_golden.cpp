#include <gtest/gtest.h>

#include "data/Database.h"
#include "data/MenuRepository.h"
#include "support/SchemaPath.h"

#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace Lms;

namespace {
Role roleFromGolden(const std::string& value) {
    if (value == "STUDENT") return Role::Student;
    if (value == "LECTURER") return Role::Lecturer;
    if (value == "HOD") return Role::HeadOfDepartment;
    if (value == "ADMIN") return Role::Administrator;
    throw std::runtime_error("Unknown role in menu golden file");
}
}

TEST(MenuGolden, ExistingRoleActionsMatchBaseline) {
    Database database(":memory:");
    std::ifstream schema(TestSupport::schemaPath());
    if (!schema) throw std::runtime_error("Cannot open configured schema file");
    std::stringstream sql;
    sql << schema.rdbuf();
    database.execute(sql.str());

    SqliteMenuRepository menus(database);
    menus.initializeDefaults();

    std::ifstream golden(std::string(LMS_GOLDEN_DIR) + "/menu_actions.txt");
    if (!golden) throw std::runtime_error("Cannot open menu golden file");
    std::map<Role, std::vector<std::string>> expected;
    std::string line;
    while (std::getline(golden, line)) {
        const auto first = line.find('|');
        const auto second = line.find('|', first + 1);
        if (first == std::string::npos || second == std::string::npos)
            throw std::runtime_error("Malformed menu golden record");
        expected[roleFromGolden(line.substr(0, first))].push_back(
            line.substr(first + 1, second - first - 1) + "|" + line.substr(second + 1));
    }

    for (const auto role : {Role::Student}) {
        std::vector<std::string> actual;
        for (const auto& item : menus.findByRole(role))
            actual.push_back(item.actionCode + "|" + item.label);
        EXPECT_EQ(actual, expected[role]);
    }
}
