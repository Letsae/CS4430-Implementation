# Requirements Traceability

The repository contains no SRS containing Must requirement identifiers. `MUST-PLACEHOLDER-*` IDs below are temporary identifiers and should be replaced when the SRS is supplied. Existing FR and DR identifiers are retained where present in the project.

| Requirement ID | Requirement | Implementation | Verification |
|---|---|---|---|
| FR1 | Role actions are restricted by role | `src/domain/User.cpp`, application service actor checks | `tests/unit/test_user.cpp`, service tests |
| FR2 | Authenticate before granting access | `src/application/AuthService.cpp` | `tests/unit/test_auth.cpp` |
| FR3 | Admin creates users | `src/application/AdminService.cpp`, `src/presentation/ConsoleUI.cpp` | `tests/integration/test_admin_assignment.cpp` |
| FR4 | Admin assigns lecturers and enrols students | `src/application/AdminService.cpp`, SQLite repositories | `tests/integration/test_admin_assignment.cpp` |
| FR5 | Admin manages departments and courses | `src/application/AdminService.cpp` | `tests/integration/test_admin_assignment.cpp` |
| FR6 | Lecturer creates assignments | `src/application/AssignmentService.cpp`, `src/presentation/ConsoleUI.cpp` | `tests/integration/test_assignment_creation.cpp` |
| FR7 | Lecturer grants extensions (to one student or, from v1.1, a whole class) | `src/application/ExtensionService.cpp` | `ModerationWorkflowIntegrationTest.EndToEndWorkflow` |
| FR7a (v1.1, CR1) | Lecturer grants one extension to every active student enrolled in a course; atomic; never shortens a later deadline | `src/application/ExtensionService.cpp` (`grantClassExtension`), `src/presentation/ConsoleUI.cpp` | `tests/unit/test_extension_service.cpp`, `tests/integration/test_class_extension.cpp` (`ClassGrant*`) |
| FR7b (v1.1, CR1) | Student assignment list shows extended deadline, original deadline and reason | `src/application/StudentService.cpp`, `src/presentation/ConsoleUI.cpp` | `ClassExtensionIntegrationTest.StudentList*`, `StudentServiceIntegrationTest.EffectiveDeadlineWithExtension` |
| FR8 | Lecturer marks submissions | `src/application/MarkingService.cpp` | `tests/integration/test_marking_service.cpp` |
| FR9 | Student submission content is bounded | `src/application/StudentService.cpp` | `tests/integration/test_student.cpp` |
| FR10 | Late submissions are accepted and flagged | `src/application/StudentService.cpp` | `tests/integration/test_student.cpp` |
| FR11 | Late penalties follow the approved penalty policy | `src/domain/LatePenaltyStrategy.cpp` | `tests/unit/test_penalty.cpp` |
| FR12 | HoD moderates marks | `src/application/ModerationService.cpp` | `tests/integration/test_moderation_service.cpp` |
| DR1 | Student sees marks only after release | `src/application/StudentService.cpp` | `tests/integration/test_marking_service.cpp`, `tests/integration/test_moderation_service.cpp` |
| DR2 | 10% per started late day for days 1-3; 0% thereafter | `src/domain/LatePenaltyStrategy.cpp` | `tests/unit/test_penalty.cpp` |
| DR3 | Extension sets effective deadline | `src/application/ExtensionService.cpp`, `src/application/StudentService.cpp`, `src/application/MarkingService.cpp` | `StudentServiceIntegrationTest.DeadlineAndExtensionBoundariesUseStartedDays`, `ModerationWorkflowIntegrationTest.EndToEndWorkflow`, `ClassExtensionIntegrationTest.Submission*` (class-extended students) |
| DR4 | Resubmission only through effective deadline | `src/application/StudentService.cpp` | `tests/integration/test_student.cpp` |
| DR5 | Each mark change is audited with actor/time/old/new/reason | `src/application/MarkingService.cpp`, `src/application/ModerationService.cpp`, `src/data/MarkAuditRepository.cpp` | Marking and moderation integration tests |
| DR6 | Each role is scoped to its own data | Application services | Unit and integration service tests |
| MUST-PLACEHOLDER-ADMIN | Administrator provisioning workflow | `src/application/AdminService.cpp`, `src/presentation/ConsoleUI.cpp` | Admin integration tests, `ModerationWorkflowIntegrationTest.EndToEndWorkflow` |
| MUST-PLACEHOLDER-LECTURER | Assignment, submission review, marking and extension workflow | `src/application/AssignmentService.cpp`, `src/application/MarkingService.cpp`, `src/application/ExtensionService.cpp` | Lecturer service integration tests, `ModerationWorkflowIntegrationTest.EndToEndWorkflow` |
| MUST-PLACEHOLDER-HOD | Department review, moderation, release and audit workflow | `src/application/ModerationService.cpp` | `tests/integration/test_moderation_service.cpp` |
| MUST-PLACEHOLDER-STUDENT | Student course, submission and released-mark workflow | `src/application/StudentService.cpp` | Student integration and golden tests |

## Schema additions and design updates

The approved additions in `db/schema.sql` add unique indexes for assignment titles per course, one extension per assignment/student, and one submission per assignment/student. Triggers make `mark_audit` append-only by rejecting updates and deletes. The design description should document these constraints and the transaction boundary for mark release.

## Change history

| Release | Change request | Requirements affected | Notes |
|---|---|---|---|
| v1.1.0 | CR1 - class-wide extensions and extension display | FR7 (modified), FR7a, FR7b (new); DR2, DR3, DR4, FR10, FR11 re-verified | No schema change. See `CHANGE_REQUEST_CR1.md`. |
