#include "BcryptPasswordHasher.h"

#include <bcrypt/BCrypt.hpp>

#include <functional>
#include <iomanip>
#include <sstream>
#include <string>

namespace Lms {
namespace {

// Compatibility verifier for the legacy format:
// $simple$<32 hex chars of salt>$<16 hex chars of std::hash-based digest>
//
// This is intentionally verification-only. New passwords are always stored
// using bcrypt. Once a legacy password verifies, AuthService rehashes it.
std::string legacySimpleHash(const std::string& input) {
    std::hash<std::string> hasher;
    size_t hashValue = hasher(input);

    for (int i = 0; i < 1000; ++i) {
        hashValue = hasher(std::to_string(hashValue) + input);
    }

    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << hashValue;
    return ss.str();
}

bool isHexString(const std::string& value) {
    for (unsigned char c : value) {
        if (!((c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F'))) {
            return false;
        }
    }
    return true;
}

bool verifyLegacySimpleHash(const std::string& password, const std::string& encoded) {
    constexpr char prefix[] = "$simple$";
    constexpr size_t prefixLength = sizeof(prefix) - 1;
    constexpr size_t saltHexLength = 32; // 16 bytes
    constexpr size_t digestLength = 16;

    // Exact format length prevents malformed/truncated hashes from being parsed.
    if (encoded.size() != prefixLength + saltHexLength + 1 + digestLength ||
        encoded.compare(0, prefixLength, prefix) != 0) {
        return false;
    }

    const size_t separator = prefixLength + saltHexLength;
    if (encoded[separator] != '$') {
        return false;
    }

    const std::string saltHex = encoded.substr(prefixLength, saltHexLength);
    const std::string storedDigest = encoded.substr(separator + 1);
    if (!isHexString(saltHex) || !isHexString(storedDigest)) {
        return false;
    }

    std::string salt;
    salt.reserve(saltHexLength / 2);
    for (size_t i = 0; i < saltHexLength; i += 2) {
        const unsigned int byte =
            static_cast<unsigned int>(std::stoul(saltHex.substr(i, 2), nullptr, 16));
        salt.push_back(static_cast<char>(byte));
    }

    const std::string computedDigest = legacySimpleHash(salt + password);
    return computedDigest == storedDigest;
}

} // namespace

BcryptPasswordHasher::BcryptPasswordHasher(int cost, size_t minimumLength, size_t maximumLength)
    : m_cost(cost), m_minimumLength(minimumLength), m_maximumLength(maximumLength) {
    if (cost < 4 || cost > 31) {
        throw ValidationException("Bcrypt cost factor must be between 4 and 31");
    }
    if (minimumLength == 0 || minimumLength > maximumLength || maximumLength > 72) {
        throw ValidationException("Password length settings must satisfy 1 <= minimum <= maximum <= 72");
    }
}

void BcryptPasswordHasher::validatePasswordLength(const std::string& password) {
    if (password.size() < m_minimumLength) {
        throw ValidationException("Password must be at least " + std::to_string(m_minimumLength) + " bytes");
    }
    if (password.size() > m_maximumLength) {
        throw ValidationException("Password must not exceed " + std::to_string(m_maximumLength) + " bytes");
    }
}

std::string BcryptPasswordHasher::hashPassword(const std::string& password) {
    validatePasswordLength(password);
    return BCrypt::generateHash(password, m_cost);
}

bool BcryptPasswordHasher::verifyPassword(const std::string& password, const std::string& hash) {
    validatePasswordLength(password);

    if (hash.rfind("$simple$", 0) == 0) {
        return verifyLegacySimpleHash(password, hash);
    }

    if (hash.size() != 60 || hash.rfind("$2", 0) != 0) {
        return false;
    }

    return BCrypt::validatePassword(password, hash);
}

} // namespace Lms
