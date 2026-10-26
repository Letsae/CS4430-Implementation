-- LMS Database Schema
-- PRAGMA settings
PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

-- Department table
CREATE TABLE IF NOT EXISTS department (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE
);

-- App User table
CREATE TABLE IF NOT EXISTS app_user (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE,
    role TEXT NOT NULL CHECK(role IN ('STUDENT', 'LECTURER', 'HOD', 'ADMIN')),
    password_hash TEXT NOT NULL,
    department_id INTEGER,
    active INTEGER NOT NULL DEFAULT 1,
    FOREIGN KEY (department_id) REFERENCES department(id),
    CHECK (department_id IS NOT NULL OR role != 'HOD')
);

-- Course table
CREATE TABLE IF NOT EXISTS course (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    code TEXT NOT NULL UNIQUE,
    title TEXT NOT NULL,
    department_id INTEGER NOT NULL,
    FOREIGN KEY (department_id) REFERENCES department(id)
);

-- Enrolment table (student-course many-to-many)
CREATE TABLE IF NOT EXISTS enrolment (
    course_id INTEGER NOT NULL,
    student_id INTEGER NOT NULL,
    enrolled_on TEXT NOT NULL,
    PRIMARY KEY (course_id, student_id),
    FOREIGN KEY (course_id) REFERENCES course(id) ON DELETE CASCADE,
    FOREIGN KEY (student_id) REFERENCES app_user(id) ON DELETE CASCADE
);

-- Course-Lecturer table (course-lecturer many-to-many)
CREATE TABLE IF NOT EXISTS course_lecturer (
    course_id INTEGER NOT NULL,
    lecturer_id INTEGER NOT NULL,
    PRIMARY KEY (course_id, lecturer_id),
    FOREIGN KEY (course_id) REFERENCES course(id) ON DELETE CASCADE,
    FOREIGN KEY (lecturer_id) REFERENCES app_user(id) ON DELETE CASCADE
);

-- Assignment table
CREATE TABLE IF NOT EXISTS assignment (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    course_id INTEGER NOT NULL,
    title TEXT NOT NULL,
    deadline TEXT NOT NULL,
    max_mark REAL NOT NULL,
    created_by INTEGER NOT NULL,
    FOREIGN KEY (course_id) REFERENCES course(id) ON DELETE CASCADE,
    FOREIGN KEY (created_by) REFERENCES app_user(id)
);

-- Extension table
CREATE TABLE IF NOT EXISTS extension (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    assignment_id INTEGER NOT NULL,
    student_id INTEGER NOT NULL,
    new_deadline TEXT NOT NULL,
    reason TEXT NOT NULL,
    granted_by INTEGER NOT NULL,
    FOREIGN KEY (assignment_id) REFERENCES assignment(id) ON DELETE CASCADE,
    FOREIGN KEY (student_id) REFERENCES app_user(id),
    FOREIGN KEY (granted_by) REFERENCES app_user(id)
);

-- Submission table
CREATE TABLE IF NOT EXISTS submission (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    assignment_id INTEGER NOT NULL,
    student_id INTEGER NOT NULL,
    state TEXT NOT NULL CHECK(state IN ('SUBMITTED', 'LATE', 'MARKED', 'MODERATED', 'RELEASED')),
    raw_mark REAL DEFAULT 0,
    penalty_percent REAL DEFAULT 0,
    final_mark REAL DEFAULT 0,
    feedback TEXT,
    first_submitted_at TEXT,
    FOREIGN KEY (assignment_id) REFERENCES assignment(id) ON DELETE CASCADE,
    FOREIGN KEY (student_id) REFERENCES app_user(id)
);

-- Submission Version table
CREATE TABLE IF NOT EXISTS submission_version (
    submission_id INTEGER NOT NULL,
    version_no INTEGER NOT NULL,
    content TEXT NOT NULL,
    submitted_at TEXT NOT NULL,
    PRIMARY KEY (submission_id, version_no),
    FOREIGN KEY (submission_id) REFERENCES submission(id) ON DELETE CASCADE
);

-- Mark Audit table (append-only)
CREATE TABLE IF NOT EXISTS mark_audit (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    submission_id INTEGER NOT NULL,
    changed_by INTEGER NOT NULL,
    changed_at TEXT NOT NULL,
    old_value REAL NOT NULL,
    new_value REAL NOT NULL,
    reason TEXT NOT NULL CHECK(LENGTH(reason) > 0),
    FOREIGN KEY (submission_id) REFERENCES submission(id) ON DELETE CASCADE,
    FOREIGN KEY (changed_by) REFERENCES app_user(id)
);

-- Notification table
CREATE TABLE IF NOT EXISTS notification (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    message TEXT NOT NULL,
    created_at TEXT NOT NULL,
    is_read INTEGER NOT NULL DEFAULT 0,
    FOREIGN KEY (user_id) REFERENCES app_user(id) ON DELETE CASCADE
);

-- Settings table (application configuration)
CREATE TABLE IF NOT EXISTS settings (
    key TEXT NOT NULL PRIMARY KEY,
    value TEXT NOT NULL
);

-- Menu item table
CREATE TABLE IF NOT EXISTS menu_item (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    action_code TEXT NOT NULL UNIQUE,
    label TEXT NOT NULL
);

-- Role menu item table (maps roles to menu items with ordering)
CREATE TABLE IF NOT EXISTS role_menu_item (
    role TEXT NOT NULL CHECK(role IN ('STUDENT', 'LECTURER', 'HOD', 'ADMIN')),
    menu_item_id INTEGER NOT NULL REFERENCES menu_item(id),
    position INTEGER NOT NULL,
    PRIMARY KEY (role, menu_item_id),
    UNIQUE (role, position)
);

-- Indexes
CREATE UNIQUE INDEX IF NOT EXISTS one_hod_per_department ON app_user(department_id) WHERE role = 'HOD';
CREATE INDEX IF NOT EXISTS idx_submission_assignment ON submission(assignment_id);
CREATE INDEX IF NOT EXISTS idx_enrolment_student ON enrolment(student_id);
CREATE INDEX IF NOT EXISTS idx_audit_submission ON mark_audit(submission_id);
