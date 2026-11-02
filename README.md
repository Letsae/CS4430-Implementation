# Moderated Learning Management System (LMS)

A console-based Learning Management System for secondary schools, supporting four user roles: Administrator, Lecturer, Head of Department, and Student.

## Features
- Role-based access control (Admin, Lecturer, HoD, Student)
- Assignment creation and submission management
- Late submission penalty calculation (10% per started day for days 1-3, no penalty from day 4)
- Extension granting by lecturers
- Mark moderation by Head of Department
- Audit trail for all mark changes (append-only)
- SQLite database persistence with WAL mode
- Password hashing (see the current implementation status in `docs/STATIC_ANALYSIS.md`)
- Observer pattern for event notifications
- State machine for submission lifecycle

## Requirements

### Build Dependencies
- CMake >= 3.16
- C++17 compatible compiler (GCC 9+, Clang 10+, or MSVC 2019+)
- SQLite3 development library
- GoogleTest (automatically fetched via CMake)

### Runtime Dependencies
- SQLite3

## Build, test, and run

The project requires CMake 3.16+, a C++17 compiler, SQLite3, and GoogleTest (fetched by CMake).

### Linux
```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y cmake g++ libsqlite3-dev

# Configure and build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/src/lms_app --db demo.db --seed-demo
```

### Windows (MSVC)
```powershell
# Install CMake from https://cmake.org/download/
# Install SQLite3 from https://www.sqlite.org/download.html

# Configure and build (using Visual Studio Developer Command Prompt)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
.\build\src\Debug\lms_app.exe --db demo.db --seed-demo
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
./build/tests/lms_unit_tests
```

### Integration Tests
Integration tests use real SQLite `:memory:` databases and test data access layer.

```bash
./build/tests/lms_integration_tests
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

### Password Hashing
Bcrypt was selected because its adjustable work factor slows offline guessing and its encoded hash embeds a unique salt. The application uses the vendored `libbcrypt` implementation; its configured work factor and embedded salts are exercised by the password hasher tests. Bcrypt is intentionally not described as memory-hard.

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
