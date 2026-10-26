#include "AuthService.h"
#include "domain/LmsException.h"
#include <thread>
#include <chrono>

namespace Lms {

AuthService::AuthService(IUserRepository& userRepo, IPasswordHasher& hasher)
    : m_userRepo(userRepo), m_hasher(hasher) {}

std::unique_ptr<User> AuthService::login(const std::string& username, const std::string& password) {
    try {
        std::string storedHash = m_userRepo.getPasswordHash(username);
        auto user = m_userRepo.getByUsername(username);

        if (!m_hasher.verifyPassword(password, storedHash)) {
            m_failedAttempts++;
            if (m_failedAttempts >= MAX_LOGIN_ATTEMPTS) {
                // Add delay to prevent brute force
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
            throw AuthenticationException("Invalid username or password");
        }

        m_failedAttempts = 0;
        m_currentUser = std::move(user);
        return std::unique_ptr<User>(UserFactory::create(
            m_currentUser->getRole(),
            m_currentUser->getId(),
            m_currentUser->getUsername()
        ));

    } catch (const NotFoundException&) {
        m_failedAttempts++;
        if (m_failedAttempts >= MAX_LOGIN_ATTEMPTS) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        throw AuthenticationException("Invalid username or password");
    }
}

void AuthService::changePassword(int userId, const std::string& oldPassword, const std::string& newPassword, const std::string& confirmPassword) {
    // Check if passwords match
    if (newPassword != confirmPassword) {
        throw ValidationException("New password and confirmation do not match");
    }

    // Get user to verify old password
    auto user = m_userRepo.getById(userId);
    std::string storedHash = m_userRepo.getPasswordHash(user->getUsername());

    if (!m_hasher.verifyPassword(oldPassword, storedHash)) {
        throw AuthenticationException("Old password is incorrect");
    }

    // Check if new password is different from old
    if (oldPassword == newPassword) {
        throw ValidationException("New password must be different from old password");
    }

    // Hash new password
    std::string newHash = m_hasher.hashPassword(newPassword);

    // Update in database
    m_userRepo.setPasswordHash(userId, newHash);
}

void AuthService::logout() {
    m_currentUser = nullptr;
}

} // namespace Lms
