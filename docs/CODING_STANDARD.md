# LMS Coding Standard

## Overview
This document defines the coding standards for the Moderated Learning Management System project. All code must adhere to these standards to ensure consistency, maintainability, and safety.

## Language and Compiler
- **Language**: C++17
- **Compiler**: GCC 9+ or Clang 10+ (Linux), MSVC 2019+ (Windows)
- **CMake**: Minimum version 3.16

## Code Style

### Naming Conventions
- **Types (classes, structs, enums)**: PascalCase (e.g., `User`, `SubmissionService`)
- **Functions**: camelCase (e.g., `getRole()`, `calculatePenalty()`)
- **Variables**: camelCase (e.g., `studentId`, `finalMark`)
- **Member variables**: camelCase with `m_` prefix (e.g., `m_studentId`, `m_finalMark`)
- **Constants**: UPPER_SNAKE_CASE (e.g., `MAX_PASSWORD_LENGTH`)
- **Files**: PascalCase matching the class name (e.g., `User.h`, `SubmissionService.cpp`)

### File Organization
- One class per header file
- Header files use `.h` extension
- Implementation files use `.cpp` extension
- Include guards in headers: `#pragma once`
- No `using namespace` in header files
- Include order: standard library, third-party, project headers

### Formatting
- Indentation: 4 spaces (no tabs)
- Line length: Maximum 120 characters
- Opening braces on same line for functions, classes, control structures
- Space after keywords (if, while, for, return)
- Spaces around operators
- No trailing whitespace

## C++ Core Guidelines Compliance

### Memory Management
- **RAII only**: No raw `new`/`delete` operations
- Use smart pointers: `std::unique_ptr` for exclusive ownership, `std::shared_ptr` for shared ownership
- Use `std::make_unique` and `std::make_shared` for creating smart pointers
- Prefer stack allocation over heap allocation when possible

### Const-Correctness
- Mark all member functions that do not modify object state as `const`
- Pass objects by `const&` when not modified
- Use `const` iterators when not modifying containers
- Mark variables as `const` when their value should not change

### Type Safety
- Use `auto` only when the type is obvious from context
- Prefer explicit types over `auto` for function return types
- Use `enum class` instead of plain enums
- Avoid C-style casts; use `static_cast`, `dynamic_cast`, `const_cast`, or `reinterpret_cast`

### Error Handling
- Use exceptions for error handling (defined in exception hierarchy)
- Never use exceptions for control flow
- Catch exceptions by `const&`
- Resource cleanup via RAII (no manual cleanup in catch blocks)

### Functions
- Maximum function length: ~40 lines
- Maximum parameter count: 5 parameters (consider struct for more)
- Functions should do one thing well
- Prefer `constexpr` for compile-time constants
- Use `nodiscard` for functions where return value must be checked

### Classes
- Prefer composition over inheritance
- Use `virtual` only when polymorphism is required
- Declare destructors `virtual` in base classes with virtual functions
- Use `override` keyword when overriding virtual functions
- Keep classes focused (Single Responsibility Principle)

## Best Practices

### Dependencies
- Dependencies point downwards only (Presentation → Application → Domain → Data)
- Upper layers own repository interfaces
- No circular dependencies
- Use forward declarations in headers when possible

### Security
- Never concatenate user input into SQL strings
- Use prepared statements with bound parameters
- Never log, print, or store plain-text passwords
- Validate all input at service boundaries
- Use constant-time comparison for password verification

### Testing
- Write tests alongside code, not after
- Unit tests use fake in-memory repositories
- Integration tests use real SQLite `:memory:`
- Test both success and failure paths
- Aim for >= 70% line coverage of application + domain layers

### Documentation
- Document public interfaces in headers
- Use Doxygen-style comments for APIs
- Comment non-obvious logic
- Keep comments up to date with code changes

## Static Analysis
- Run `clang-tidy` and fix all warnings
- Run `cppcheck` and fix all warnings
- Maintain reports in `docs/` directory

## Git Commit Messages
- Use conventional commit format: `<type>(<scope>): <description>`
- Types: `feat`, `fix`, `refactor`, `test`, `docs`, `chore`, `style`
- Examples:
  - `feat(domain): add User hierarchy with role-based permissions`
  - `fix(auth): resolve bcrypt password verification issue`
  - `test(submission): add penalty boundary tests`

## Prohibited Patterns
- Raw pointers without smart pointer wrapper
- Global variables or singletons
- Magic numbers (use named constants)
- `using namespace` in headers
- C-style casts
- Multiple inheritance (except for interface-only classes)
- Variable-length arrays (VLAs)
- `goto` statements
- Exceptions for normal control flow
