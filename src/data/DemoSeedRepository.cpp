#include "DemoSeedRepository.h"
#include "domain/LmsException.h"

#include <fstream>
#include <sstream>

namespace Lms {

DemoSeedRepository::DemoSeedRepository(Database& database) : m_database(database) {}

void DemoSeedRepository::loadSqlFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw DatabaseException("Could not open configured demo seed file");
    std::ostringstream sql;
    sql << file.rdbuf();
    m_database.execute(sql.str());
}

std::vector<DemoSeedRecord> DemoSeedRepository::getRecords() const {
    constexpr const char* query = "SELECT kind, first_value, second_value, third_value "
                                 "FROM temp.demo_seed ORDER BY rowid;";
    sqlite3_stmt* statement = nullptr;
    const int prepareResult = sqlite3_prepare_v2(m_database.get(), query, -1, &statement, nullptr);
    if (prepareResult != SQLITE_OK)
        throw DatabaseException("Could not read demo seed data: " + std::string(sqlite3_errmsg(m_database.get())));

    std::vector<DemoSeedRecord> records;
    int stepResult = SQLITE_ROW;
    while ((stepResult = sqlite3_step(statement)) == SQLITE_ROW) {
        const auto column = [statement](int index) {
            const auto* value = sqlite3_column_text(statement, index);
            return value ? std::string(reinterpret_cast<const char*>(value)) : std::string();
        };
        records.push_back({column(0), column(1), column(2), column(3)});
    }
    sqlite3_finalize(statement);
    if (stepResult != SQLITE_DONE)
        throw DatabaseException("Could not read demo seed data: " + std::string(sqlite3_errmsg(m_database.get())));
    return records;
}

} // namespace Lms
