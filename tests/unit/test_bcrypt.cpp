#include <gtest/gtest.h>
#include "security/BcryptPasswordHasher.h"
#include "domain/LmsException.h"

using namespace Lms;

class BcryptPasswordHasherTest : public ::testing::Test {
protected:
    BcryptPasswordHasher m_hasher;
};

TEST_F(BcryptPasswordHasherTest, HashesPasswordSuccessfully) {
    std::string password = "SecurePassword123";
    std::string hash = m_hasher.hashPassword(password);

    EXPECT_FALSE(hash.empty());
    EXPECT_NE(hash, password);
    EXPECT_TRUE(hash.find("$simple$") == 0); // Simple hash prefix
}

TEST_F(BcryptPasswordHasherTest, VerifiesCorrectPassword) {
    std::string password = "SecurePassword123";
    std::string hash = m_hasher.hashPassword(password);

    EXPECT_TRUE(m_hasher.verifyPassword(password, hash));
}

TEST_F(BcryptPasswordHasherTest, RejectsIncorrectPassword) {
    std::string password = "SecurePassword123";
    std::string wrongPassword = "WrongPassword456";
    std::string hash = m_hasher.hashPassword(password);

    EXPECT_FALSE(m_hasher.verifyPassword(wrongPassword, hash));
}

TEST_F(BcryptPasswordHasherTest, DifferentHashesForSamePassword) {
    std::string password = "SecurePassword123";
    std::string hash1 = m_hasher.hashPassword(password);
    std::string hash2 = m_hasher.hashPassword(password);

    // Hashes should be different due to random salt
    EXPECT_NE(hash1, hash2);

    // But both should verify correctly
    EXPECT_TRUE(m_hasher.verifyPassword(password, hash1));
    EXPECT_TRUE(m_hasher.verifyPassword(password, hash2));
}

TEST_F(BcryptPasswordHasherTest, RejectsShortPassword) {
    std::string shortPassword = "short";
    EXPECT_THROW(m_hasher.hashPassword(shortPassword), ValidationException);
}

TEST_F(BcryptPasswordHasherTest, RejectsLongPassword) {
    std::string longPassword(73, 'a'); // 73 characters
    EXPECT_THROW(m_hasher.hashPassword(longPassword), ValidationException);
}

TEST_F(BcryptPasswordHasherTest, AcceptsMinimumLengthPassword) {
    std::string password = "12345678"; // Exactly 8 characters
    EXPECT_NO_THROW(m_hasher.hashPassword(password));
}

TEST_F(BcryptPasswordHasherTest, AcceptsMaximumLengthPassword) {
    std::string password(72, 'a'); // Exactly 72 characters
    EXPECT_NO_THROW(m_hasher.hashPassword(password));
}

TEST(BcryptPasswordHasherCost, RejectsInvalidCostFactor) {
    EXPECT_THROW(BcryptPasswordHasher(3), ValidationException); // Too low
    EXPECT_THROW(BcryptPasswordHasher(32), ValidationException); // Too high
}

TEST(BcryptPasswordHasherCost, AcceptsValidCostFactor) {
    EXPECT_NO_THROW(BcryptPasswordHasher(4)); // Minimum
    EXPECT_NO_THROW(BcryptPasswordHasher(12)); // Default
    EXPECT_NO_THROW(BcryptPasswordHasher(31)); // Maximum
}
