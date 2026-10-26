#pragma once

#include <stdexcept>
#include <string>

namespace Lms {

/**
 * Base exception class for all LMS-specific exceptions.
 */
class LmsException : public std::runtime_error {
public:
    explicit LmsException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * Exception thrown when input validation fails.
 */
class ValidationException : public LmsException {
public:
    explicit ValidationException(const std::string& message)
        : LmsException(message) {}
};

/**
 * Exception thrown when authentication fails.
 */
class AuthenticationException : public LmsException {
public:
    explicit AuthenticationException(const std::string& message)
        : LmsException(message) {}
};

/**
 * Exception thrown when authorization fails (user lacks permission).
 */
class AuthorisationException : public LmsException {
public:
    explicit AuthorisationException(const std::string& message)
        : LmsException(message) {}
};

/**
 * Exception thrown when an invalid state transition is attempted.
 */
class InvalidTransitionException : public LmsException {
public:
    explicit InvalidTransitionException(const std::string& message)
        : LmsException(message) {}
};

/**
 * Exception thrown when resubmission is attempted after deadline.
 */
class ResubmissionClosedException : public LmsException {
public:
    explicit ResubmissionClosedException(const std::string& message)
        : LmsException(message) {}
};

/**
 * Exception thrown when a requested entity is not found.
 */
class NotFoundException : public LmsException {
public:
    explicit NotFoundException(const std::string& message)
        : LmsException(message) {}
};

/**
 * Exception thrown when a database operation fails.
 */
class DatabaseException : public LmsException {
public:
    explicit DatabaseException(const std::string& message)
        : LmsException(message) {}
};

} // namespace Lms
