#pragma once
#include "data/Database.h"
#include <string>

namespace Lms {

/**
 * Interface for settings repository.
 * Stores application configuration values in the database.
 */
class ISettingsRepository {
public:
    virtual ~ISettingsRepository() = default;

    /**
     * Get a setting value by key.
     * @param key The setting key
     * @return The setting value
     * @throws NotFoundException if key doesn't exist
     */
    virtual std::string get(const std::string& key) = 0;

    /**
     * Get a setting value with default.
     * @param key The setting key
     * @param defaultValue The default value if key doesn't exist
     * @return The setting value or default
     */
    virtual std::string getWithDefault(const std::string& key, const std::string& defaultValue) = 0;

    /**
     * Set a setting value.
     * @param key The setting key
     * @param value The setting value
     */
    virtual void set(const std::string& key, const std::string& value) = 0;

    /**
     * Check if a setting exists.
     * @param key The setting key
     * @return true if exists, false otherwise
     */
    virtual bool exists(const std::string& key) = 0;

    /**
     * Initialize default settings if they don't exist.
     */
    virtual void initializeDefaults() = 0;
};

/**
 * SQLite implementation of settings repository.
 */
class SqliteSettingsRepository : public ISettingsRepository {
public:
    explicit SqliteSettingsRepository(Database& db);
    ~SqliteSettingsRepository() override = default;

    std::string get(const std::string& key) override;
    std::string getWithDefault(const std::string& key, const std::string& defaultValue) override;
    void set(const std::string& key, const std::string& value) override;
    bool exists(const std::string& key) override;
    void initializeDefaults() override;

private:
    Database& m_db;
};

} // namespace Lms
