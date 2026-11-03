# LMS Installation Guide

**Project:** CS4430 Moderated Learning Management System (LMS)  
**Repository:** https://github.com/Letsae/CS4430-Implementation  
**Target platform:** Windows 11  
**Release:** v1.1.0 (use only after the release tag exists)

## 1. Purpose and deployment assumptions

This guide describes building and running the LMS on a Windows 11 school computer. It assumes a **local console application with a local SQLite database**. If the intended deployment is multiple computers sharing one database, this guide and the deployment diagram must be revised before installation.

## 2. Prerequisites

Install or make available:

- Windows 11
- Git for Windows
- CMake
- A C++17-compatible compiler (for example, Visual Studio Build Tools with the C++ workload)
- SQLite development/runtime dependencies required by the project
- The project's bundled bcrypt dependency, if included in the repository

Use the versions specified by the project’s CMake configuration or course environment. Do not download or replace dependencies with unverified versions.

## 3. Obtain the source code

Open PowerShell and run:

```powershell
git clone https://github.com/Letsae/CS4430-Implementation.git
cd CS4430-Implementation
```

For a tagged release, check out the tag after it has been created:

```powershell
git checkout v1.1.0
```

If the tag has not yet been created, remain on the approved release branch/commit and record that commit in the installation record.

## 4. Configure and build

From the repository root:

```powershell
cmake -S . -B build
cmake --build build 
```

If CMake reports a missing compiler or dependency, install the prerequisite and rerun configuration. Do not continue if the build fails.

## 5. Run automated tests

```powershell
ctest --test-dir build --output-on-failure
```

**Acceptance check:** all registered tests should pass. If a test fails, record the failure and resolve it before declaring the installation verified.

## 6. Database setup

The LMS uses SQLite. The database is local to the installation unless the implementation has been explicitly configured otherwise.

1. Locate the database initialization/schema instructions in the repository.
2. Follow the project's actual schema creation and initial administrator setup procedure.
3. Confirm the database file is created in the expected location.
4. Back up the database before importing or changing production data.

> **Important:** The exact database filename, schema command, seed-data command, and first-administrator procedure must be confirmed from the current source/configuration before this guide is used operationally. Do not assume a demo-seeding command exists.

## 7. Launch the application

Find the executable produced by the Release build. The exact executable name and path depend on the project's CMake target configuration.

Run the executable from PowerShell or the project's documented launch location. For example, after confirming the actual executable path:

```powershell
./build/src/lms_app.exe  
```


