#include "BcryptPasswordHasher.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <functional>
#include <algorithm>

namespace Lms {

// Simple SHA-256-like hash using standard library (for demonstration only)
// In production, use a proper cryptographic library like OpenSSL or libsodium
static std::string simpleHash(const std::string& input) {
    std::hash<std::string> hasher;
    size_t hashValue = hasher(input);

    // Mix the hash multiple times for better distribution
    for (int i = 0; i < 1000; ++i) {
        hashValue = hasher(std::to_string(hashValue) + input);
    }

    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << hashValue;
    return ss.str();
}

BcryptPasswordHasher::BcryptPasswordHasher(int cost)
    : m_cost(cost) {
    if (cost < 4 || cost > 31) {
        throw ValidationException("Bcrypt cost factor must be between 4 and 31");
    }
}

void BcryptPasswordHasher::validatePasswordLength(const std::string& password) {
    if (password.length() < 8) {
        throw ValidationException("Password must be at least 8 characters");
    }
    if (password.length() > 72) {
        throw ValidationException("Password must not exceed 72 characters");
    }
}

std::string BcryptPasswordHasher::hashPassword(const std::string& password) {
    validatePasswordLength(password);

    // Generate random salt
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    std::string salt;
    for (int i = 0; i < 16; ++i) {
        salt += static_cast<char>(dis(gen));
    }

    // Hash password + salt
    std::string saltedPassword = salt + password;
    std::string hash = simpleHash(saltedPassword);

    // Encode as hex string with salt prefix
    std::stringstream ss;
    ss << "$simple$";

    // Encode salt as hex
    for (char c : salt) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(static_cast<unsigned char>(c));
    }
    ss << "$";
    ss << hash;

    return ss.str();
}

bool BcryptPasswordHasher::verifyPassword(const std::string& password, const std::string& hash) {
    validatePasswordLength(password);

    // Parse the hash format: $simple$<salt>$<hash>
    size_t saltStart = hash.find('$', 1) + 1;
    size_t hashStart = hash.find('$', saltStart) + 1;

    if (saltStart == std::string::npos || hashStart == std::string::npos) {
        return false;
    }

    std::string saltHex = hash.substr(saltStart, hashStart - saltStart - 1);
    std::string storedHash = hash.substr(hashStart);

    // Convert hex salt back to bytes
    std::string salt;
    for (size_t i = 0; i < saltHex.length(); i += 2) {
        std::string byteStr = saltHex.substr(i, 2);
        salt += static_cast<char>(std::stoi(byteStr, nullptr, 16));
    }

    // Hash the password with the same salt
    std::string saltedPassword = salt + password;
    std::string computedHash = simpleHash(saltedPassword);

    // Compare hashes
    return computedHash == storedHash;
}

} // namespace Lms
