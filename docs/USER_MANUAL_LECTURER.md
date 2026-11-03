# User Manual — Lecturer

## 1. Role and responsibilities

Lecturers manage their assigned courses and assignments, review student submissions, and mark work. A lecturer should only access courses assigned to their account.

## 2. Sign in

1. Start the LMS.
2. Enter your lecturer username and password.
3. Confirm that the Lecturer menu appears.

## 3. Main tasks

### View assigned courses
1. Choose the course-list or course-management option.
2. Select a course assigned to you.
3. Review its information and available assignments.

### Create an assignment
1. Open the assignment-management option.
2. Choose the create-assignment operation.
3. Enter the title, description/instructions, deadline, and any other fields requested by the application.
4. Review the details before confirming.
5. Verify the assignment appears under the correct course.

Only enter fields supported by the current application. Confirm the deadline and time format carefully.

### View submissions
1. Select the relevant course and assignment.
2. Open the submission-list option.
3. Review submitted work and its status.
4. Follow the application's available workflow for opening or reviewing a submission.

### Mark work and provide feedback
1. Select the submission.
2. Enter the mark and feedback requested by the system.
3. Review the values before saving.
4. Confirm the result was saved.
5. If the LMS uses a moderation workflow, ensure the mark is submitted for moderation rather than treated as released.

### Extend a deadline (one student or the whole class)
1. Choose **Grant Extension** from the Lecturer menu.
2. Select the assignment.
3. Choose the scope: **1. One student** or **2. Whole class** (every active student enrolled in the assignment's course).
4. For one student, select the student. For the whole class, no student selection is needed.
5. Enter the new deadline in ISO 8601 UTC (for example `2035-06-05T12:00:00Z`) and a reason. The new deadline must be later than the current deadline.
6. Read the confirmation. For a class extension it reports how many students were extended; students who already had the same or a later deadline are left unchanged, and inactive students are skipped.

Extensions apply to the chosen assignment only. Each affected student is notified and sees the extension in their assignment list. An extension cannot be shortened or removed once granted. A class extension is all-or-nothing: if anything fails, no student is changed.

### Manage late submissions
Check the assignment deadline and the system's displayed late-submission or penalty information. Do not manually override a penalty unless the application provides an authorized workflow and the action is justified.

## 4. Good practice

- Explain assignment requirements and deadlines clearly.
- Verify the correct student and assignment before marking.
- Save feedback that is constructive and relevant.
- Keep marking decisions consistent with course policy.
- Do not share student submissions or marks with unauthorized people.

## 5. Troubleshooting

- **Course is missing:** contact the Administrator to verify your course assignment.
- **Assignment cannot be created:** check required fields and deadline format.
- **Submission cannot be marked:** check submission status and whether the role is authorized.
- **Mark is awaiting moderation:** follow the HoD moderation process.

> **Documentation check before submission:** Confirm the exact assignment fields, submission statuses, and menu labels against the tested v1.1.0 build.
