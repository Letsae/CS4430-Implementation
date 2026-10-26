#include <gtest/gtest.h>
#include "data/Database.h"
#include "data/CourseRepository.h"
#include "data/AssignmentRepository.h"
#include "data/SubmissionRepository.h"
#include "data/SubmissionVersionRepository.h"
#include "data/ExtensionRepository.h"
#include "data/DepartmentRepository.h"
#include "data/UserRepository.h"
#include "data/EnrolmentRepository.h"
#include "application/StudentService.h"
#include "domain/LmsException.h"
#include "domain/User.h"
#include "util/Clock.h"
#include <memory>

using namespace Lms;

class StudentServiceIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        db = std::make_unique<Database>(":memory:");
        
        // Create tables
        db->execute(
            "CREATE TABLE department (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL UNIQUE);"
            "CREATE TABLE app_user (id INTEGER PRIMARY KEY AUTOINCREMENT, username TEXT NOT NULL UNIQUE, role TEXT NOT NULL, password_hash TEXT NOT NULL, department_id INTEGER, active INTEGER NOT NULL DEFAULT 1);"
            "CREATE TABLE course (id INTEGER PRIMARY KEY AUTOINCREMENT, code TEXT NOT NULL UNIQUE, title TEXT NOT NULL, department_id INTEGER NOT NULL);"
            "CREATE TABLE enrolment (course_id INTEGER NOT NULL, student_id INTEGER NOT NULL, enrolled_on TEXT NOT NULL, PRIMARY KEY (course_id, student_id));"
            "CREATE TABLE course_lecturer (course_id INTEGER NOT NULL, lecturer_id INTEGER NOT NULL, PRIMARY KEY (course_id, lecturer_id));"
            "CREATE TABLE assignment (id INTEGER PRIMARY KEY AUTOINCREMENT, course_id INTEGER NOT NULL, title TEXT NOT NULL, deadline TEXT NOT NULL, max_mark REAL NOT NULL, created_by INTEGER NOT NULL);"
            "CREATE TABLE submission (id INTEGER PRIMARY KEY AUTOINCREMENT, assignment_id INTEGER NOT NULL, student_id INTEGER NOT NULL, state TEXT NOT NULL, raw_mark REAL DEFAULT 0, penalty_percent REAL DEFAULT 0, final_mark REAL DEFAULT 0, feedback TEXT, first_submitted_at TEXT);"
            "CREATE TABLE extension (id INTEGER PRIMARY KEY AUTOINCREMENT, assignment_id INTEGER NOT NULL, student_id INTEGER NOT NULL, new_deadline TEXT NOT NULL, reason TEXT NOT NULL, granted_by INTEGER NOT NULL);"
            "CREATE TABLE submission_version (submission_id INTEGER NOT NULL, version_no INTEGER NOT NULL, content TEXT NOT NULL, submitted_at TEXT NOT NULL, PRIMARY KEY (submission_id, version_no));"
        );

        departmentRepo = std::make_unique<SqliteDepartmentRepository>(*db);
        userRepo = std::make_unique<SqliteUserRepository>(*db);
        courseRepo = std::make_unique<SqliteCourseRepository>(*db);
        assignmentRepo = std::make_unique<SqliteAssignmentRepository>(*db);
        submissionRepo = std::make_unique<SqliteSubmissionRepository>(*db);
        versionRepo = std::make_unique<SqliteSubmissionVersionRepository>(*db);
        extensionRepo = std::make_unique<SqliteExtensionRepository>(*db);
        enrolmentRepo = std::make_unique<SqliteEnrolmentRepository>(*db, *courseRepo, *userRepo);

        clock = std::make_unique<FixedClock>("2024-01-15T10:00:00Z");

        studentService = std::make_unique<StudentService>(
            *courseRepo, *assignmentRepo, *submissionRepo, *versionRepo, *extensionRepo,
            *departmentRepo, *userRepo, *clock
        );
        
        // Setup test data
        setupTestData();
    }
    
    void setupTestData() {
        // Create departments
        Department csDept(0, "Computer Science");
        int csDeptId = departmentRepo->save(csDept);
        
        // Create users
        std::string passwordHash = "$simple$test$test"; // Simplified for testing
        userRepo->saveWithPassword("student1", Role::Student, passwordHash, csDeptId);
        userRepo->saveWithPassword("student2", Role::Student, passwordHash, csDeptId);
        userRepo->saveWithPassword("lecturer1", Role::Lecturer, passwordHash, csDeptId);
        
        // Create courses
        Course course1(0, "CS101", "Intro to Programming", csDeptId);
        int courseId = courseRepo->save(course1);
        courseRepo->assignLecturer(courseId, 3); // lecturer1
        
        // Enroll students
        enrolmentRepo->enroll(courseId, 1); // student1
        enrolmentRepo->enroll(courseId, 2); // student2
        
        // Create assignment
        Assignment assign(0, courseId, "Assignment 1", "2024-01-20T10:00:00Z", 100.0, 1);
        assignmentRepo->save(assign);
    }
    
    std::unique_ptr<Database> db;
    std::unique_ptr<SqliteDepartmentRepository> departmentRepo;
    std::unique_ptr<SqliteUserRepository> userRepo;
    std::unique_ptr<SqliteCourseRepository> courseRepo;
    std::unique_ptr<SqliteAssignmentRepository> assignmentRepo;
    std::unique_ptr<SqliteSubmissionRepository> submissionRepo;
    std::unique_ptr<SqliteSubmissionVersionRepository> versionRepo;
    std::unique_ptr<SqliteExtensionRepository> extensionRepo;
    std::unique_ptr<SqliteEnrolmentRepository> enrolmentRepo;
    std::unique_ptr<FixedClock> clock;
    std::unique_ptr<StudentService> studentService;
};

TEST_F(StudentServiceIntegrationTest, ListCoursesReturnsOnlyEnrolledCourses) {
    auto courses = studentService->listCourses(1);
    
    ASSERT_EQ(courses.size(), 1);
    EXPECT_EQ(courses[0].code, "CS101");
    EXPECT_EQ(courses[0].title, "Intro to Programming");
    EXPECT_EQ(courses[0].department, "Computer Science");
    ASSERT_EQ(courses[0].lecturers.size(), 1u);
    EXPECT_EQ(courses[0].lecturers[0], "lecturer1");
}

TEST_F(StudentServiceIntegrationTest, ListCoursesReturnsEmptyForUnenrolledStudent) {
    // Create a student3 who is not enrolled
    std::string passwordHash = "$simple$test$test";
    userRepo->saveWithPassword("student3", Role::Student, passwordHash, 1);
    
    auto courses = studentService->listCourses(3);
    
    EXPECT_TRUE(courses.empty());
}

TEST_F(StudentServiceIntegrationTest, ListSubmissionsReturnsOnlyStudentSubmissions) {
    // Create a submission for student1
    Submission sub(0, 1, 1, SubmissionState::Submitted);
    sub.setFirstSubmittedAt("2024-01-15T10:00:00Z");
    submissionRepo->save(sub);
    
    auto submissions = studentService->listSubmissions(1);
    
    ASSERT_EQ(submissions.size(), 1);
    EXPECT_EQ(submissions[0].state, "Submitted");
    EXPECT_EQ(submissions[0].courseCode, "CS101");
    EXPECT_EQ(submissions[0].assignmentTitle, "Assignment 1");
}

TEST_F(StudentServiceIntegrationTest, ListSubmissionsReturnsEmptyForStudentWithNoSubmissions) {
    auto submissions = studentService->listSubmissions(2);
    
    EXPECT_TRUE(submissions.empty());
}

TEST_F(StudentServiceIntegrationTest, MarkAndFeedbackHiddenWhenNotReleased) {
    // Create a submission with mark but not released state
    Submission sub(0, 1, 1, SubmissionState::Marked);
    sub.setFirstSubmittedAt("2024-01-15T10:00:00Z");
    sub.setFinalMark(85.0);
    sub.setFeedback("Good work");
    submissionRepo->save(sub);
    
    auto submissions = studentService->listSubmissions(1);
    
    ASSERT_EQ(submissions.size(), 1);
    EXPECT_FALSE(submissions[0].markReleased);
    EXPECT_EQ(submissions[0].feedback, "Mark not yet released");
}

TEST_F(StudentServiceIntegrationTest, MarkAndFeedbackShownWhenReleased) {
    // Create a submission with released state
    Submission sub(0, 1, 1, SubmissionState::Released);
    sub.setFirstSubmittedAt("2024-01-15T10:00:00Z");
    sub.setFinalMark(85.0);
    sub.setFeedback("Good work");
    submissionRepo->save(sub);
    
    auto submissions = studentService->listSubmissions(1);
    
    ASSERT_EQ(submissions.size(), 1);
    EXPECT_TRUE(submissions[0].markReleased);
    EXPECT_EQ(submissions[0].mark, 85.0);
    EXPECT_EQ(submissions[0].feedback, "Good work");
}

TEST_F(StudentServiceIntegrationTest, EffectiveDeadlineWithExtension) {
    // Create an extension for student1
    Extension ext(0, 1, 1, "2024-01-25T10:00:00Z", "Medical", 1);
    extensionRepo->save(ext);
    
    auto courses = studentService->listCourses(1);
    
    ASSERT_EQ(courses.size(), 1);
    ASSERT_EQ(courses[0].assignments.size(), 1);
    EXPECT_TRUE(courses[0].assignments[0].hasExtension);
    EXPECT_EQ(courses[0].assignments[0].effectiveDeadline, "2024-01-25T10:00:00Z");
}

TEST_F(StudentServiceIntegrationTest, SubmitRejectsEmptyContent) {
    EXPECT_THROW(studentService->submit(1, 1, ""), ValidationException);
}

TEST_F(StudentServiceIntegrationTest, SubmitRejectsTooLongContent) {
    std::string longContent(20001, 'a');
    EXPECT_THROW(studentService->submit(1, 1, longContent), ValidationException);
}

TEST_F(StudentServiceIntegrationTest, SubmitRejectsNotEnrolled) {
    // student3 is not enrolled in any course
    EXPECT_THROW(studentService->submit(3, 1, "Some content"), AuthorisationException);
}
