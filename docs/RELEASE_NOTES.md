# LMS Release Notes

## Release 1.1.0

**Release tag:** `v1.1.0`  
**Based on:** `v1.0.0`  
**Change request:** CR1 (see [`CHANGE_REQUEST_CR1.md`](CHANGE_REQUEST_CR1.md))  
**Maintenance type:** Perfective  
**Release date:** [Enter actual release date]  

### What is new

- **Class-wide extensions (FR7a).** In the Lecturer menu, **Grant Extension** now asks whether to extend for **one student** or the **whole class**. A class extension gives every active student enrolled in the assignment's course the same new deadline and reason, in one all-or-nothing operation. Students who already have the same or a later deadline are left unchanged and reported; inactive students are skipped. Each affected student receives one notification.
- **Extensions on the student's assignment list (FR7b).** The list now reads `Deadline: <new> (extended from <original>)` followed by `Extension reason: <reason>`. Assignments without an extension look the same as before.

### Changed files

`ExtensionService` (new `grantClassExtension`), `StudentService`/`AssignmentInfo` (new `extensionReason`), `ConsoleUI` (scope prompt, list display), version bump to 1.1.0. No database schema change.

### Upgrade notes

- No data migration: existing v1.0.0 databases work unchanged. Replace the executable and rebuild (`cmake -S . -B build`, `cmake --build build`).
- Lecturer workflow change: one extra prompt after choosing an assignment (`1. One student`, `2. Whole class`). The single-student path behaves as in v1.0.0.

### Verification (regression)

| Suite | v1.0.0 | v1.1.0 |
|---|---:|---:|
| Unit | 102 / 102 | 113 / 113 |
| Integration | 33 / 33 | 49 / 49 |
| Total | 135 / 135 | 162 / 162 (27 new, 0 failed) |

All 135 pre-existing tests pass unchanged. Evidence: `docs/regression/baseline_v1.0.txt`, `docs/regression/results_v1.1.txt`. Re-run with `ctest --test-dir build --output-on-failure` on the release commit before tagging.

### Known limitations (v1.1.0)

Class scope is the whole course roster (use the single-student option for subsets); one reason for all students; no revoke/shorten; no confirmation prompt before a class grant; the console UI has no automated tests.

### Release checklist (v1.1.0)

- [ ] Official CMake/GoogleTest build and `ctest` pass on the release commit (162 tests).
- [ ] `MANUAL_TEST.md` steps 5a-5c pass.
- [ ] Git tag `v1.1.0` is created on the approved release commit.
- [ ] Release date and commit hash are recorded.

---

## Release 1.0.0

**Release tag:** `v1.0.0`  
**Release date:** [Enter actual release date]  
**Project:** CS4430 Moderated Learning Management System  
**Repository:** https://github.com/Letsae/CS4430-Implementation

> **Release status:** Draft for final verification. Do not label this release production-ready or tag it until the checklist below is completed.

## Overview

Version 1.0.0 is intended to provide a console-based LMS for the school workflow, with role-based access for Administrator, Lecturer, Head of Department (HoD), and Student. Confirm each listed capability against the final release build before publishing these notes.

## Intended capabilities to verify

- User authentication and role-specific menus.
- Administrator management of users, departments, courses, lecturer assignments, and student enrolment (where implemented).
- Lecturer assignment creation, submission review, marking, and feedback (where implemented).
- HoD moderation, mark adjustment/approval, audit records, and mark release (where implemented).
- Student course/assignment access, submission or resubmission, and viewing released marks/feedback (where implemented).
- SQLite persistence.
- Password hashing using bcrypt.
- Audit logging for relevant moderation actions.
- Deadline and late-submission penalty handling.

## Verification and testing

Previously reported test/coverage figures from the development conversation:

- Line coverage: **78.4% (925 / 1,180 lines)**
- Function coverage: **82.3% (205 / 249 functions)**

These figures are historical and must be re-run against the exact release commit before publication. Coverage is not a substitute for acceptance testing.

### Release checklist

- [ ] All required unit and integration tests pass on the release commit.
- [ ] Manual acceptance tests for all four roles pass.
- [ ] Database initialization and persistence are verified.
- [ ] Authentication and role permissions are verified.
- [ ] Moderation and audit-log behavior are verified.
- [ ] Installation guide matches the final build and database setup.
- [ ] All four role manuals match actual menu labels and workflows.
- [ ] Known issues and limitations are documented.
- [ ] Git tag `v1.0.0` is created on the approved release commit.
- [ ] Release date and commit hash are recorded.

## Known limitations / items to confirm

- Exact executable name and launch path.
- Exact database file path and first-administrator setup.
- Whether deployment is single-computer/local or networked.
- Exact submission input format and any file-upload support.
- Any incomplete features, bugs, or restrictions identified during acceptance testing.

## Installation

See [`INSTALLATION_GUIDE.md`](INSTALLATION_GUIDE.md).

## User documentation

- [`USER_MANUAL_ADMIN.md`](USER_MANUAL_ADMIN.md)
- [`USER_MANUAL_LECTURER.md`](USER_MANUAL_LECTURER.md)
- [`USER_MANUAL_HOD.md`](USER_MANUAL_HOD.md)
- [`USER_MANUAL_STUDENT.md`](USER_MANUAL_STUDENT.md)
