#pragma once

#include "domain/User.h"
#include "data/UserRepository.h"
#include "security/IPasswordHasher.h"
#include <memory>
#include <string>

namespace Lms {

/**
 * Authentication service.
 * Handles login, logout, and password changes.
 */
class AuthService {
public:
    AuthService(IUserRepository& userRepo, IPasswordHasher& hasher);

    /**
     * Authenticate a user with username and password.
     * @param username The username
     * @param password The plain-text password
     * @return The authenticated user
     * @throws AuthenticationException if authentication fails
     */
    std::unique_ptr<User> login(const std::string& username, const std::string& password);

    /**
     * Change the password for a user.
     * @param userId The user ID
     * @param oldPassword The current password
     * @param newPassword The new password
     * @param confirmPassword Confirmation of new password
     * @throws AuthenticationException if old password is incorrect
     * @throws ValidationException if new password is invalid or doesn't match confirmation
     */
    void changePassword(int userId, const std::string& oldPassword, const std::string& newPassword, const std::string& confirmPassword);

    /**
     * Logout the current user.
     */
    void logout();

    /**
     * Get the currently logged-in user.
     */
    User* getCurrentUser() const { return m_currentUser.get(); }

private:
    IUserRepository& m_userRepo;
    IPasswordHasher& m_hasher;
    std::unique_ptr<User> m_currentUser;

    static const int MAX_LOGIN_ATTEMPTS = 5;
    int m_failedAttempts = 0;
};

} // namespace Lms
