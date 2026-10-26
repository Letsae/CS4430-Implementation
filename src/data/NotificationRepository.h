#pragma once

#include "domain/Entities.h"
#include "data/Database.h"
#include "data/IRepository.h"

namespace Lms {

class INotificationRepository : public IRepository<Notification> {
public:
    virtual ~INotificationRepository() = default;
};

class SqliteNotificationRepository : public INotificationRepository {
public:
    explicit SqliteNotificationRepository(Database& db);
    std::unique_ptr<Notification> getById(int id) override;
    std::vector<Notification> getAll() override;
    int save(const Notification& entity) override;
    void remove(int id) override;
    bool exists(int id) override;
private:
    Database& m_db;
};

} // namespace Lms
