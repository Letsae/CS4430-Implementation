#pragma once

#include "Database.h"
#include <string>
#include <vector>

namespace Lms {

struct DemoSeedRecord {
    std::string kind;
    std::string first;
    std::string second;
    std::string third;
};

class DemoSeedRepository {
public:
    explicit DemoSeedRepository(Database& database);
    void loadSqlFile(const std::string& path);
    std::vector<DemoSeedRecord> getRecords() const;

private:
    Database& m_database;
};

} // namespace Lms
