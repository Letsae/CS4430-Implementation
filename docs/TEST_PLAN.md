# Phase 5: Verification and Validation

## 1. Test Strategy

### 1.1 Objectives

The purpose of testing is to verify that the Moderated Learning Management System (LMS) conforms to its requirements, design, security rules and business logic.

Testing will verify:

- Role-based authorisation for Administrator, Lecturer, Head of Department (HoD) and Student.
- Assignment creation, submission, resubmission and deadline handling.
- Late-penalty calculations and extensions.
- Marking, moderation, mark release and student mark visibility.
- Audit logging of mark changes.
- SQLite persistence and exception handling.
- End-to-end operation through the console application.

GoogleTest is used for automated unit and integration testing. GitHub Actions builds the project, runs the tests and generates a coverage report.

### 1.2 V-Model test plan

| V-Model verification activity | Corresponding validation activity | Artefact verified | Entry criteria | Exit criteria |
|---|---|---|---|---|
| Requirements analysis | Acceptance testing | Software Requirements Specification (SRS), use cases and stakeholder requirements | Requirements are documented and testable | All critical requirements have acceptance tests and results |
| System design | System testing | Architecture, role permissions, submission lifecycle and system design | Design and system requirements are approved | System tests pass; critical system defects are resolved |
| Architectural design | Integration testing | Service interfaces, repository interfaces, SQLite integration and component interactions | Components and interfaces are implemented and individually tested | Integration tests pass; data flow and interactions work correctly |
| Detailed design | Unit testing | Domain classes, application services, late-penalty strategy, validation and authorisation logic | Code is implemented and dependencies are available | Unit tests pass; business rules and boundary cases are verified |

### 1.3 Test environment

| Component | Test environment |
|---|---|
| Programming language | C++17 |
| Build system | CMake |
| Testing framework | GoogleTest |
| Database | SQLite |
| CI platform | GitHub Actions |
| Coverage tool | lcov |
| Operating system | Ubuntu GitHub Actions runner; Windows development environment |

Tests should use generated test data and fresh SQLite in-memory databases where appropriate. Test cases must not depend on hard-coded database IDs.

---

## 2. Equivalence Partitioning and Boundary Value Analysis

### 2.1 Deadline and late-penalty testing

The effective deadline is the student's extension deadline when an extension exists; otherwise, it is the original assignment deadline.

The penalty is 10% of the maximum mark for each started 24-hour period of lateness, up to three days. From the fourth day, the awarded mark is zero. Late submissions are still accepted.

Let:

- \(M\) = maximum assignment mark
- \(d\) = time elapsed after the effective deadline

The penalty is:

- On time: no penalty.
- Late by up to 24 hours: 10% of \(M\).
- Late by more than 24 and up to 48 hours: 20% of \(M\).
- Late by more than 48 and up to 72 hours: 30% of \(M\).
- More than 72 hours late: zero awarded mark.

| Test ID | Input class / boundary | Expected result |
|---|---|---|
| DL01 | Submission before deadline | On time; no penalty |
| DL02 | Submission exactly at deadline | On time; no penalty |
| DL03 | Deadline +1 second | Late; 10% penalty |
| DL04 | Deadline +24 hours | Late; 10% penalty |
| DL05 | Deadline +24 hours +1 second | Late; 20% penalty |
| DL06 | Deadline +48 hours | Late; 20% penalty |
| DL07 | Deadline +72 hours | Late; 30% penalty |
| DL08 | Deadline +72 hours +1 second | Late; zero awarded mark |
| DL09 | Submission exactly at extension deadline | On time relative to extension |
| DL10 | Submission +1 second after extension deadline | Late; first-day penalty |
| DL11 | Submission after original deadline but before extension deadline | On time; no penalty |
| DL12 | Submission after effective deadline on day four | Accepted; zero awarded mark |

**Note:** The test implementation must confirm the precise interpretation of the 24-hour boundary. The expected values above follow the project requirement's started-24-hour-period rule.

### 2.2 Mark validation

| Test ID | Equivalence class / boundary | Expected result |
|---|---|---|
| MK01 | Mark = 0 | Accepted |
| MK02 | Mark = maximum mark | Accepted |
| MK03 | Mark = maximum mark + 0.01 | Rejected |
| MK04 | Negative mark | Rejected |
| MK05 | Valid mark with feedback | Accepted |
| MK06 | Invalid mark supplied during marking | Validation exception; no invalid mark persisted |

### 2.3 Role and authorisation testing

| Test ID | Role / action | Expected result |
|---|---|---|
| AU01 | Administrator creates user or course | Allowed |
| AU02 | Lecturer creates assignment for an authorised course | Allowed |
| AU03 | Lecturer attempts action outside assigned course | Authorisation exception |
| AU04 | HoD moderates mark in own department | Allowed |
| AU05 | HoD accesses another department's marks | Authorisation exception |
| AU06 | Student views own submissions | Allowed |
| AU07 | Student attempts lecturer, HoD or admin action | Authorisation exception |
| AU08 | Unauthenticated user invokes protected service | Authorisation exception |

---

## 3. Automated Unit and Integration Testing

### 3.1 Unit testing

Unit tests verify individual classes and application services in isolation.

| Test area | Test examples |
|---|---|
| LatePenaltyStrategy | Deadline boundaries, penalty calculation and day-four zero mark |
| AuthService | Valid login, invalid credentials, authorisation and password validation |
| AssignmentService | Assignment validation, deadline and maximum mark |
| SubmissionService | Submission validation, resubmission and version handling |
| MarkingService | Valid marks, invalid marks and feedback |
| ModerationService | Mark approval, adjustment and reason validation |
| StudentService | Mark visibility before and after release |
| Authorisation | Allowed and denied actions for each role |

Unit tests should use fake repositories where suitable to isolate business logic.

### 3.2 Integration testing

Integration tests verify interactions between application services and SQLite repositories.

| Test ID | Integration test | Expected result |
|---|---|---|
| IT01 | Create and retrieve user through service and repository | User persists correctly |
| IT02 | Create assignment and retrieve it from SQLite | Assignment persists |
| IT03 | Submit assignment and retrieve submission | Submission persists |
| IT04 | Resubmit assignment | New version is stored; latest version is identifiable |
| IT05 | Mark submission and retrieve mark | Mark and feedback persist |
| IT06 | Moderate mark and inspect audit repository | Mark change and audit entry persist |
| IT07 | Attempt invalid database operation | Exception handled; no partial invalid state |
| IT08 | Use SQL injection-like input in a string field | Input treated as data; database remains intact |

Integration tests should use fresh SQLite `:memory:` databases and verify persistence, transaction rollback where applicable, and repository behaviour.

---

## 4. System and Acceptance Testing

### 4.1 System test cases

System tests verify the complete application through its services, repositories and console interface.

| Test ID | Scenario | Expected result |
|---|---|---|
| ST01 | Administrator creates department, users and course | Records are stored and roles are assigned correctly |
| ST02 | Lecturer creates assignment | Assignment is saved with deadline and maximum mark |
| ST03 | Student submits on time | Submission is accepted and marked on time |
| ST04 | Student submits late | Submission is accepted and correct penalty is applied |
| ST05 | Student resubmits before effective deadline | New version is stored; latest version is marked |
| ST06 | Student attempts resubmission after deadline | Submission is rejected |
| ST07 | Lecturer marks submission | Mark and feedback are saved |
| ST08 | Student views mark before release | Mark is not visible |
| ST09 | HoD moderates and releases mark | Mark becomes visible after release |
| ST10 | Lecturer or HoD accesses unauthorised course or department | Access is rejected |
| ST11 | Database operation fails | Exception is handled without application crash |
| ST12 | Invalid mark is entered | Validation error occurs; invalid mark is not persisted |

### 4.2 Acceptance test: EndToEndWorkflow

**Objective:** Demonstrate that the complete LMS assessment workflow satisfies the stakeholder requirements.

**Preconditions:**

- A fresh SQLite database is available.
- The application and repositories are operational.
- The test creates its own users, department, course and assignment.

**Test procedure:**

1. Administrator creates a department, HoD, lecturer, two students and a course.
2. Administrator assigns the lecturer and enrols both students.
3. Lecturer creates an assignment with a deadline and maximum mark.
4. Student A submits on time and resubmits before the deadline, creating version 2.
5. Student B submits after the deadline within the first 24-hour late period.
6. A separate student submits after the original deadline but before an approved extension deadline.
7. Students attempt to view marks before marking and release.
8. Lecturer marks the submissions and provides feedback.
9. Students attempt to view marks again before moderation and release.
10. HoD reviews the marks, approves Student A's mark and adjusts Student B's mark with a reason.
11. Students attempt to view marks before release.
12. HoD releases the approved marks.
13. Students view the released marks and feedback.
14. Verify that the late penalty is correct.
15. Verify that every mark change has an audit record containing the actor, timestamp, old mark, new mark and reason.
16. Verify that the audit log cannot be modified through application operations.
17. Attempt cross-course and cross-department access and confirm that it is rejected.
18. Confirm that a Student cannot invoke Lecturer, HoD or Administrator actions.

**Expected result:** The complete workflow succeeds, marks remain hidden until release, late penalties are correct, resubmission versions are preserved, and all mark changes are audited.

---

## 5. Test Results and Coverage

### 5.1 Automated CI results

The project uses GitHub Actions to configure, build, test and generate coverage reports.

| Metric | Actual result |
|---|---:|
| Business-logic line coverage | 78.4% |
| Lines covered | 925 / 1,180 |
| Function coverage | 82.3% |
| Functions covered | 205 / 249 |
| Required line coverage | At least 70% |
| Coverage requirement | Satisfied |

The business-logic coverage report includes the application and domain layers.

**Evidence:** GitHub Actions `build-and-test` workflow and lcov coverage output.

### 5.2 Test execution status

| Test level | Status |
|---|---|
| Unit tests | Automated through GoogleTest and CI |
| Integration tests | Automated through GoogleTest and CI |
| Coverage measurement | Completed; 78.4% line coverage |
| System test execution | Record actual execution results before submission |
| Acceptance test execution | Record actual execution results before submission |

Do not mark system or acceptance tests as passed until the test scenarios have actually been executed and their results recorded.

---

## 6. Defect Log

The following defects were observed during CI development and resolved.

| ID | Severity | Defect / cause | Fix | Retest result |
|---|---|---|---|---|
| D01 | High | CI configuration failed because the SQLite imported target was unavailable under the runner's CMake configuration | Updated SQLite linking to use the discovered SQLite libraries and include directories | Passed; CMake configuration and build succeeded |
| D02 | High | bcrypt source files could not locate required header files in CI | Corrected bcrypt include directories in CMake | Passed; bcrypt compiled successfully |
| D03 | Medium | CI workflow contained YAML indentation errors and duplicated coverage steps | Corrected indentation and removed duplicate coverage step | Passed; coverage workflow completed successfully |

Severity is assessed according to the impact on the build, test execution and application functionality.

---

## 7. Conclusion

The LMS has automated GoogleTest execution through GitHub Actions and achieved 78.4% business-logic line coverage, exceeding the minimum required 70%.

The V-Model test strategy defines unit, integration, system and acceptance testing activities. Boundary-value and equivalence-partitioning tests cover deadlines, late penalties, extensions, marks and role authorisation.

System and acceptance test results must be recorded after execution to demonstrate that the complete LMS satisfies its functional requirements.

---

## 8. Maintenance release v1.1.0 - CR1 tests and regression (class-wide extensions)

### 8.1 New test cases

| Test ID | Scenario | Expected result | Automated test |
|---|---|---|---|
| CR1-01 | Lecturer grants a class extension | One extension row per active enrolled student, with the new deadline, reason and granting lecturer | `ClassGrantCreatesOneExtensionPerEnrolledStudent` |
| CR1-02 | Students of other courses / other assignments | No extension created | `ClassGrantDoesNotTouchOtherCoursesOrOtherAssignments` |
| CR1-03 | Inactive student in class | Skipped and counted | `ClassGrantSkipsInactiveStudents` |
| CR1-04 | Student already holds a later extension | Kept unchanged (never shortened) | `ClassGrantNeverShortensAnExistingLaterExtension` |
| CR1-05 | Student holds an earlier extension | Updated in place; still one row per student | `ClassGrantUpdatesEarlierExtensionsInPlace` |
| CR1-06 | Nobody would receive a later deadline | Validation error | `ClassGrantRejectedWhenNobodyWouldGetALaterDeadline` |
| CR1-07 | Blank reason, empty deadline, wrong lecturer, student actor | Error; nothing saved | `ClassGrantRejectsBadInputAndWrongLecturerWithoutSavingAnything`, `ClassExtensionReject*` (unit) |
| CR1-08 | Notifications | Exactly one per affected student | `ClassGrantNotifiesEachAffectedStudentOnce` |
| CR1-09 | Notification failure during a class grant | Full rollback; no extension saved | `ClassGrantIsAtomicWhenAnObserverFailsMidway` |
| CR1-10 | Student assignment list for an extended assignment | Shows original deadline, extended deadline, reason | `StudentListShowsOriginalExtendedDeadlineAndReason` |
| CR1-11 | Assignment list without extension / unaffected students | Unchanged | `StudentListUnchangedForUnextendedAssignments`, `StudentListShowsExtensionOnlyToAffectedStudents` |
| CR1-12 | Submission between original and class deadline | On time, 0% penalty | `SubmissionBetweenOriginalAndClassDeadlineIsOnTime` |
| CR1-13 | Submission 1 second after class deadline | Late, 10% penalty | `SubmissionOneSecondAfterClassDeadlineIsLateWithFirstDayPenalty` |
| CR1-14 | Student outside the class | Original deadline still applies | `StudentOutsideTheClassStillUsesOriginalDeadline` |
| CR1-15 | Single-student grant after the validation refactor | Same behaviour and error order as v1.0.0 | `SingleGrant*` (unit and integration) |

### 8.2 Regression results

| Suite | v1.0.0 baseline | v1.1.0 | Failed |
|---|---:|---:|---:|
| Unit | 102 / 102 | 113 / 113 | 0 |
| Integration | 33 / 33 | 49 / 49 | 0 |
| Total | 135 / 135 | 162 / 162 | 0 |

All 135 existing tests ran unchanged and passed. The 27 new tests passed, and a mutation check (removing the never-shorten guard and the transaction) made three of them fail, confirming sensitivity. Evidence: `docs/regression/baseline_v1.0.txt` and `docs/regression/results_v1.1.txt`. These runs used a sandbox harness (GoogleTest-compatible shim over the unmodified sources); repeat with `ctest --test-dir build --output-on-failure` before tagging v1.1.0.

### 8.3 System test additions

| Test ID | Scenario | Expected result |
|---|---|---|
| ST13 | Lecturer grants a class extension through the console | Summary shows the number of students affected; each receives a notification |
| ST14 | Student opens the assignment list after a class extension | Deadline shows `(extended from <original>)` and the reason |
