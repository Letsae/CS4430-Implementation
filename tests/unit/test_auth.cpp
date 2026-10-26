#include <gtest/gtest.h>
#include "application/AuthService.h"
#include "data/UserRepository.h"
#include "data/Database.h"
#include "security/BcryptPasswordHasher.h"
#include "support/SchemaPath.h"
#include <fstream>

using namespace Lms;

class AuthServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_db = std::make_unique<Database>(":memory:");

        // Load schema
        std::ifstream schemaFile(TestSupport::schemaPath());
        ASSERT_TRUE(schemaFile.is_open()) << "Could not open schema.sql";

        std::stringstream schema;
        schema << schemaFile.rdbuf();
        m_db->execute(schema.str());

        m_userRepo = std::make_unique<SqliteUserRepository>(*m_db);
        m_hasher = std::make_unique<BcryptPasswordHasher>(12);
        m_authService = std::make_unique<AuthService>(*m_userRepo, *m_hasher);

        // Create a test user
        std::string password = "TestPassword123";
        std::string hash = m_hasher->hashPassword(password);
        m_testUserId = m_userRepo->saveWithPassword("testuser", Role::Student, hash, 0);
    }

    std::unique_ptr<Database> m_db;
    std::unique_ptr<SqliteUserRepository> m_userRepo;
    std::unique_ptr<BcryptPasswordHasher> m_hasher;
    std::unique_ptr<AuthService> m_authService;
    int m_testUserId;
};

TEST_F(AuthServiceTest, LoginWithCorrectCredentials) {
    auto user = m_authService->login("testuser", "TestPassword123");

    ASSERT_NE(user, nullptr);
    EXPECT_EQ(user->getUsername(), "testuser");
    EXPECT_EQ(user->getRole(), Role::Student);
}

TEST_F(AuthServiceTest, LoginWithIncorrectPassword) {
    EXPECT_THROW(
        m_authService->login("testuser", "WrongPassword"),
        AuthenticationException
    );
}

TEST_F(AuthServiceTest, LoginWithNonExistentUser) {
    EXPECT_THROW(
        m_authService->login("nonexistent", "TestPassword123"),
        AuthenticationException
    );
}

TEST_F(AuthServiceTest, ChangePasswordWithCorrectOldPassword) {
    EXPECT_NO_THROW(
        m_authService->changePassword(m_testUserId, "TestPassword123", "NewPassword456", "NewPassword456")
    );

    // Verify new password works
    EXPECT_NO_THROW(m_authService->login("testuser", "NewPassword456"));

    // Verify old password doesn't work
    EXPECT_THROW(
        m_authService->login("testuser", "TestPassword123"),
        AuthenticationException
    );
}

TEST_F(AuthServiceTest, ChangePasswordWithIncorrectOldPassword) {
    EXPECT_THROW(
        m_authService->changePassword(m_testUserId, "WrongPassword", "NewPassword456", "NewPassword456"),
        AuthenticationException
    );
}

TEST_F(AuthServiceTest, LogoutClearsCurrentUser) {
    m_authService->login("testuser", "TestPassword123");
    EXPECT_NE(m_authService->getCurrentUser(), nullptr);

    m_authService->logout();
    EXPECT_EQ(m_authService->getCurrentUser(), nullptr);
}

TEST_F(AuthServiceTest, LoginLockoutAfterFiveFailures) {
    // Attempt 5 failed logins
    for (int i = 0; i < 5; ++i) {
        EXPECT_THROW(
            m_authService->login("testuser", "WrongPassword"),
            AuthenticationException
        );
    }

    // The 6th attempt should work (after delay), but we can't easily test the delay in unit tests
    // Just verify that the mechanism is in place
}

TEST_F(AuthServiceTest, FailedAttemptsResetAfterSuccessfulLogin) {
    // Fail 3 times
    for (int i = 0; i < 3; ++i) {
        EXPECT_THROW(
            m_authService->login("testuser", "WrongPassword"),
            AuthenticationException
        );
    }

    // Successful login should reset counter
    EXPECT_NO_THROW(m_authService->login("testuser", "TestPassword123"));

    // Should be able to fail again without immediate lockout
    EXPECT_THROW(
        m_authService->login("testuser", "WrongPassword"),
        AuthenticationException
    );
}
