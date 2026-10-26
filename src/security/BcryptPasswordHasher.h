#pragma once

#include "IPasswordHasher.h"
#include "domain/LmsException.h"
#include <string>

// Forward declaration for bcrypt structure
struct bcrypt_context;

namespace Lms {

/**
 * Bcrypt password hasher implementation.
 * Uses cost factor 12 by default (configurable).
 */
class BcryptPasswordHasher : public IPasswordHasher {
public:
    /**
     * Constructor with configurable cost factor.
     * @param cost Bcrypt cost factor (4-31, default 12)
     */
    explicit BcryptPasswordHasher(int cost = 12);

    std::string hashPassword(const std::string& password) override;
    bool verifyPassword(const std::string& password, const std::string& hash) override;

private:
    int m_cost;

    void validatePasswordLength(const std::string& password);
};

} // namespace Lms
