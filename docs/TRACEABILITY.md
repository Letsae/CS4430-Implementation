# Requirements Traceability Matrix

This document maps each Must requirement from the SRS to the code and tests that implement it.

| Requirement ID | Requirement | Code Location | Test Location |
|---------------|-------------|---------------|---------------|
| FR1 | The system shall permit each user actions of their role (Admin, Lecturer, HoD, Student) | src/domain/User.h (canPerform method) | tests/unit/test_user.cpp |
| FR2 | The system shall authenticate every user by username and password before granting access to any | src/application/AuthService.cpp (login) | tests/unit/test_auth.cpp |
| FR3 | The admin shall be able to create, edit and delete accounts. | src/application/AdminService.cpp (createUser) | *Integration test needed* |
| FR4 | The admin shall be able to assign lecturers to courses and enrol students in courses. | src/application/AdminService.cpp (assignLecturer, enrollStudent) | *Integration test needed* |
| FR5 | Admin shall be able to create, edit and delete department account and courses. | src/application/AdminService.cpp (createDepartment, createCourse) | *Integration test needed* |
| FR6 | Lecturer shall be able to create an assignment for a course with deadline and maximum mark. | *UI placeholder in ConsoleUI* | *Integration test needed* |
| FR7 | Lecturer shall be able to grant an extension. | src/data/ExtensionRepository.cpp | *Integration test needed* |
| FR8 | Lecturer shall be able to enter mark. | src/application/MarkingService.cpp (markSubmission) | *Integration test needed* |
| FR9 | Student shall be able to submit a file not larger than 40MB. | src/application/SubmissionService.cpp (submitAssignment) | *Integration test needed* |
| FR10 | After deadline the system shall accept submission and mark them as late submissions. | src/application/SubmissionService.cpp (submitAssignment) | *Integration test needed* |
| FR11 | For late submission system shall compute the final mark minus 10%* with full day late, for submissions less than 3 days late. The system shall return 0 for submissions older than 3 days after deadline | src/domain/LatePenaltyStrategy.cpp | tests/unit/test_penalty.cpp |
| FR12 | HoD shall be able to approve marked submissions or return back to lecturer with a comment. | src/application/ModerationService.cpp (moderateSubmission) | *Integration test needed* |

## Design Rules Traceability

| Design Rule | Code Location | Test Location |
|-------------|---------------|----------------
| DR1 | Student sees mark only when Released | src/presentation/ConsoleUI.cpp (showStudentMenu) | *Integration test needed* |
| DR2 | Late penalty calculation (10% per day 1-3, 0 from day 4) | src/domain/LatePenaltyStrategy.cpp | tests/unit/test_penalty.cpp |
| DR3 | Effective deadline = extension or assignment deadline | src/application/SubmissionService.cpp (getEffectiveDeadline) | *Integration test needed* |
| DR4 | Resubmission only before effective deadline | src/application/SubmissionService.cpp (canResubmit) | *Integration test needed* |
| DR5 | MarkAuditEntry for every mark change | src/application/MarkingService.cpp (writeAuditEntry) | tests/integration/test_database.cpp |
| DR6 | Role-based access control | src/domain/User.h (canPerform method) | tests/unit/test_user.cpp |
