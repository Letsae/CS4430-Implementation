#pragma once

#include <memory>
#include <vector>

namespace Lms {

/**
 * Generic repository interface for CRUD operations.
 * @tparam T Entity type
 */
template <typename T>
class IRepository {
public:
    virtual ~IRepository() = default;

    /**
     * Get an entity by ID.
     * @throws NotFoundException if not found
     */
    virtual std::unique_ptr<T> getById(int id) = 0;

    /**
     * Get all entities.
     */
    virtual std::vector<T> getAll() = 0;

    /**
     * Save an entity (insert or update).
     * @return The ID of the saved entity.
     */
    virtual int save(const T& entity) = 0;

    /**
     * Delete an entity by ID.
     */
    virtual void remove(int id) = 0;

    /**
     * Check if an entity exists by ID.
     */
    virtual bool exists(int id) = 0;
};

} // namespace Lms
