# Moderated Learning Management System (LMS)

A console-based Learning Management System for secondary schools, supporting four user roles: Administrator, Lecturer, Head of Department, and Student.

## Features
- Role-based access control (Admin, Lecturer, HoD, Student)
- Assignment creation and submission management
- Late submission penalty calculation (10% per day for days 1-3, 100% from day 4)
- Extension granting by lecturers
- Mark moderation by Head of Department
- Audit trail for all mark changes (append-only)
- SQLite database persistence with WAL mode
- Secure password hashing with bcrypt (cost factor 12)
- Observer pattern for event notifications
- State machine for submission lifecycle

## Requirements

### Build Dependencies
- CMake >= 3.16
- C++17 compatible compiler (GCC 9+, Clang 10+, or MSVC 2019+)
- SQLite3 development library
- GoogleTest (automatically fetched via CMake)
- bcrypt library (automatically fetched via CMake)

### Runtime Dependencies
- SQLite3

## Building

For full setup details, generator-specific commands, test invocation and
troubleshooting, see [docs/BUILDING.md](docs/BUILDING.md).

### Linux
```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y cmake g++ libsqlite3-dev

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run tests
cd build
ctest --output-on-failure

# Run application
./build/lms_app
```

### Windows (MSVC)
```powershell
# Install CMake from https://cmake.org/download/
# Install SQLite3 from https://www.sqlite.org/download.html

# Configure and build (using Visual Studio Developer Command Prompt)
cmake -B build -G "Visual Studio 16 2019" -A x64
cmake --build build --config Debug

# Run tests
.\build\Debug\lms_unit_tests.exe
.\build\Debug\lms_integration_tests.exe

# Run application
.\build\Debug\lms_app.exe
```

### Build with Coverage (Linux)
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage"
cmake --build build
cd build
ctest --output-on-failure
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/googletest/*' '*/tests/*' --output-file coverage.info
lcov --list coverage.info
```

## Running Tests

### Unit Tests
Unit tests use fake in-memory repositories and test business logic in isolation.

```bash
./build/lms_unit_tests
```

### Integration Tests
Integration tests use real SQLite `:memory:` databases and test data access layer.

```bash
./build/lms_integration_tests
```

### All Tests
```bash
cd build
ctest --output-on-failure
```

## Static Analysis

### clang-tidy
```bash
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
run-clang-tidy -p build src/
```

### cppcheck
```bash
cppcheck --enable=all --std=c++17 --suppress=missingIncludeSystem src/
```

## Security Justification

### Password Hashing with bcrypt
This project uses bcrypt for password hashing instead of alternatives like SHA-256 or PBKDF2 for the following reasons:

1. **Adaptive cost factor**: bcrypt's cost factor can be increased as hardware improves, making it future-proof against brute-force attacks.
2. **Built-in salt**: bcrypt automatically generates and embeds a unique salt in the hash string, eliminating salt management errors.
3. **Memory-hard**: bcrypt is designed to be slow and memory-intensive, making GPU/ASIC attacks expensive.
4. **Battle-tested**: bcrypt has been in use since 1999 and is widely trusted by security professionals.
5. **Simple API**: The bcrypt hash string contains all necessary parameters (salt, cost, hash), simplifying storage and verification.

### Library Choice
We use the `trgl/bcrypt` library (v1.0.0) which:
- Is actively maintained and regularly audited
- Follows the official bcrypt specification
- Has a stable API with pinned version in CMake
- Provides constant-time password verification to prevent timing attacks

### Password Policy
- Minimum length: 8 characters
- Maximum length: 72 characters (bcrypt limit)
- Rejected passwords outside this range
- Generic error messages to prevent username enumeration
- Account lockout/delay after 5 failed attempts

### SQL Injection Prevention
- All SQL uses prepared statements with bound parameters
- Never concatenates user input into SQL strings
- Multi-step writes wrapped in transactions with RAII rollback guard

## Project Structure
```
lms/
├── src/
│   ├── presentation/    # Console UI
│   ├── application/     # Business logic services
│   ├── domain/          # Core entities and business rules
│   ├── data/            # Repository implementations
│   ├── security/        # Password hashing
│   └── util/            # Utilities (Clock, etc.)
├── tests/
│   ├── unit/            # Unit tests with fake repositories
│   └── integration/     # Integration tests with real SQLite
├── db/
│   └── schema.sql       # Database schema
├── docs/
│   ├── CODING_STANDARD.md
│   └── TRACEABILITY.md
└── .github/workflows/   # CI configuration
```

## Architecture
The system follows a four-tier layered architecture:
1. **Presentation Layer**: Console UI with role-based menus
2. **Application Layer**: Services containing business rules and authorization
3. **Domain Layer**: Core entities (User, Course, Assignment, Submission, etc.)
4. **Data Access Layer**: Repository interfaces and SQLite implementations

Dependencies point downwards only, ensuring testability and maintainability.

## Design Patterns
- **Strategy**: ILatePenaltyStrategy with StandardLatePenalty and NoLatePenalty
- **Repository**: IRepository<T> with SQLite implementations
- **Factory**: UserFactory for creating User objects based on role
- **Observer**: EventPublisher with IObserver for event notifications
- **State**: SubmissionStateMachine for submission lifecycle

## Coding Standards
See `docs/CODING_STANDARD.md` for detailed coding standards including:
- C++ Core Guidelines compliance
- RAII memory management
- Const-correctness
- Naming conventions
- Security best practices

## Requirements Traceability
See `docs/TRACEABILITY.md` for mapping of Must requirements to code and tests.

## License
[Add license information here]
