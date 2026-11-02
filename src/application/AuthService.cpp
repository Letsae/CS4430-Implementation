#include "AuthService.h"
#include "domain/LmsException.h"

#include <chrono>

namespace Lms {
namespace {
constexpr int kDefaultMaximumLoginAttempts = 5;
constexpr int kDefaultLockoutSeconds = 5;
constexpr char kForcedPasswordChangeUserSetting[] = "force_password_change_user_id";

bool isLegacyPasswordHash(const std::string& hash) {
    return hash.rfind("$simple$", 0) == 0;
}
}

AuthService::AuthService(IUserRepository& userRepo, IPasswordHasher& hasher)
    : m_userRepo(userRepo), m_hasher(hasher) {}

AuthService::AuthService(IUserRepository& userRepo, IPasswordHasher& hasher,
                         ISettingsRepository& settings, const Clock& clock)
    : m_userRepo(userRepo), m_hasher(hasher), m_settings(&settings), m_clock(&clock) {}

int AuthService::settingInt(const std::string& name, int fallback) const {
    if (!m_settings) return fallback;
    const std::string value = m_settings->getWithDefault(name, std::to_string(fallback));
    size_t consumed = 0;
    int parsed = 0;
    try {
        parsed = std::stoi(value, &consumed);
    } catch (const std::exception&) {
        throw ValidationException("Invalid integer setting: " + name);
    }
    if (consumed != value.size() || parsed < 0)
        throw ValidationException("Invalid integer setting: " + name);
    return parsed;
}

void AuthService::recordFailure(const std::string& username) {
    const int maximumAttempts = settingInt("max_login_attempts", kDefaultMaximumLoginAttempts);
    if (maximumAttempts == 0) throw ValidationException("Maximum login attempts must be positive");
    int& attempts = m_failedAttempts[username];
    ++attempts;
    if (attempts >= maximumAttempts) {
        const int delay = settingInt("lockout_delay_seconds", kDefaultLockoutSeconds);
        m_lockedUntil[username] = m_clock->nowTimePoint() + std::chrono::seconds(delay);
    }
}

std::unique_ptr<User> AuthService::login(const std::string& username, const std::string& password) {
    const auto now = m_clock->nowTimePoint();
    const auto lock = m_lockedUntil.find(username);
    if (lock != m_lockedUntil.end() && lock->second > now)
        throw AuthenticationException("Invalid username or password");
    if (lock != m_lockedUntil.end()) m_lockedUntil.erase(lock);

    try {
        const std::string storedHash = m_userRepo.getPasswordHash(username);
        auto user = m_userRepo.getByUsername(username);
        bool verified = false;
        try {
            verified = m_hasher.verifyPassword(password, storedHash);
        } catch (const ValidationException&) {
            // Login failures, including invalid password lengths, remain generic.
        }
        if (!verified) {
            recordFailure(username);
            throw AuthenticationException("Invalid username or password");
        }

        // Upgrade a successfully verified legacy hash to bcrypt. If the write
        // fails, do not silently continue with a stale credential record.
        if (isLegacyPasswordHash(storedHash)) {
            const std::string upgradedHash = m_hasher.hashPassword(password);
            m_userRepo.setPasswordHash(user->getId(), upgradedHash);
        }

        m_failedAttempts.erase(username);
        m_lockedUntil.erase(username);
        m_currentUser = std::move(user);
        return std::unique_ptr<User>(UserFactory::create(
            m_currentUser->getRole(), m_currentUser->getId(), m_currentUser->getUsername()));
    } catch (const NotFoundException&) {
        recordFailure(username);
        throw AuthenticationException("Invalid username or password");
    }
}

void AuthService::changePassword(int userId, const std::string& oldPassword,
                                 const std::string& newPassword, const std::string& confirmPassword) {
    if (newPassword != confirmPassword)
        throw ValidationException("New password and confirmation do not match");

    auto user = m_userRepo.getById(userId);
    const std::string storedHash = m_userRepo.getPasswordHash(user->getUsername());
    bool oldPasswordMatches = false;
    try {
        oldPasswordMatches = m_hasher.verifyPassword(oldPassword, storedHash);
    } catch (const ValidationException&) {
        // Treat malformed/short old-password input as an authentication failure.
    }
    if (!oldPasswordMatches)
        throw AuthenticationException("Old password is incorrect");

    if (oldPassword == newPassword)
        throw ValidationException("New password must be different from old password");

    const std::string newHash = m_hasher.hashPassword(newPassword);
    m_userRepo.setPasswordHash(userId, newHash);
    if (m_settings && m_settings->getWithDefault(kForcedPasswordChangeUserSetting, "0") == std::to_string(userId))
        m_settings->set(kForcedPasswordChangeUserSetting, "0");
}

void AuthService::logout() { m_currentUser = nullptr; }

bool AuthService::isPasswordChangeRequired() const {
    if (!m_currentUser || !m_settings) return false;
    return m_settings->getWithDefault(kForcedPasswordChangeUserSetting, "0") ==
           std::to_string(m_currentUser->getId());
}

} // namespace Lms
