#pragma once

#include <string>

namespace Lms {

/**
 * Interface for password hashing.
 */
class IPasswordHasher {
public:
    virtual ~IPasswordHasher() = default;

    /**
     * Hash a password.
     * @param password The plain-text password (8-72 characters)
     * @return The bcrypt hash string
     * @throws ValidationException if password length is invalid
     */
    virtual std::string hashPassword(const std::string& password) = 0;

    /**
     * Verify a password against a hash.
     * @param password The plain-text password to verify
     * @param hash The bcrypt hash to verify against
     * @return true if password matches, false otherwise
     */
    virtual bool verifyPassword(const std::string& password, const std::string& hash) = 0;
};

} // namespace Lms
