#include <gtest/gtest.h>
#include "data/Database.h"
#include "data/DepartmentRepository.h"
#include "data/CourseRepository.h"
#include "data/AssignmentRepository.h"
#include "data/SubmissionRepository.h"
#include "data/MarkAuditRepository.h"
#include "domain/LmsException.h"
#include "support/SchemaPath.h"
#include "support/TestData.h"
#include <fstream>

using namespace Lms;

class DatabaseIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create in-memory database
        m_db = std::make_unique<Database>(":memory:");

        // Load schema
        std::ifstream schemaFile(TestSupport::schemaPath());
        ASSERT_TRUE(schemaFile.is_open()) << "Could not open schema.sql";

        std::stringstream schema;
        schema << schemaFile.rdbuf();
        m_db->execute(schema.str());
    }

    std::unique_ptr<Database> m_db;
};

TEST_F(DatabaseIntegrationTest, CreatesDatabaseSuccessfully) {
    ASSERT_NE(m_db, nullptr);
    ASSERT_NE(m_db->get(), nullptr);
}

TEST_F(DatabaseIntegrationTest, TransactionGuardCommitsOnSuccess) {
    {
        TransactionGuard guard(*m_db);
        // Do some work
        m_db->execute("INSERT INTO department (name) VALUES ('Test Dept');");
        guard.commit();
    }

    // Verify data was committed
    const char* sql = "SELECT COUNT(*) FROM department WHERE name = 'Test Dept';";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(m_db->get(), sql, -1, &stmt, nullptr);

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    EXPECT_EQ(count, 1);
}

TEST_F(DatabaseIntegrationTest, TransactionGuardRollbacksOnException) {
    try {
        TransactionGuard guard(*m_db);
        m_db->execute("INSERT INTO department (name) VALUES ('Test Dept');");
        throw std::runtime_error("Test exception");
    } catch (...) {
        // Expected
    }

    // Verify data was rolled back
    const char* sql = "SELECT COUNT(*) FROM department WHERE name = 'Test Dept';";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(m_db->get(), sql, -1, &stmt, nullptr);

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    EXPECT_EQ(count, 0);
}

TEST_F(DatabaseIntegrationTest, DepartmentRepositoryCrud) {
    SqliteDepartmentRepository repo(*m_db);

    // Create
    Department dept(0, "Computer Science");
    int id = repo.save(dept);
    EXPECT_GT(id, 0);

    // Read
    auto retrieved = repo.getById(id);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getId(), id);
    EXPECT_EQ(retrieved->getName(), "Computer Science");

    // Update (by saving new with same ID)
    retrieved->setName("CS Department");
    repo.save(*retrieved);

    auto updated = repo.getById(id);
    EXPECT_EQ(updated->getName(), "CS Department");
    EXPECT_THROW(repo.save(Department(0, "CS Department")), ValidationException);

    // Delete
    repo.remove(id);
    EXPECT_FALSE(repo.exists(id));
}

TEST_F(DatabaseIntegrationTest, CourseRepositoryCrud) {
    // First create a department
    SqliteDepartmentRepository deptRepo(*m_db);
    Department dept(0, "Engineering");
    int deptId = deptRepo.save(dept);

    SqliteCourseRepository repo(*m_db);

    // Create
    Course course(0, "CS101", "Intro to CS", deptId);
    int id = repo.save(course);
    EXPECT_GT(id, 0);

    // Read
    auto retrieved = repo.getById(id);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getId(), id);
    EXPECT_EQ(retrieved->getCode(), "CS101");
    EXPECT_EQ(retrieved->getTitle(), "Intro to CS");
    EXPECT_EQ(retrieved->getDepartmentId(), deptId);

    // Get by department
    auto courses = repo.getByDepartment(deptId);
    EXPECT_EQ(courses.size(), 1);
    EXPECT_EQ(courses[0].getId(), id);
}

TEST_F(DatabaseIntegrationTest, AssignmentRepositoryCrud) {
    LmsTestData::TestData data(*m_db);
    const int departmentId = data.makeDepartment();
    const int courseId = data.makeCourse(departmentId);
    const int lecturerId = data.makeUser(Role::Lecturer, departmentId);

    SqliteAssignmentRepository repo(*m_db);
    const int id = data.makeAssignment(courseId, lecturerId);
    EXPECT_GT(id, 0);

    // Read
    auto retrieved = repo.getById(id);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getId(), id);
    EXPECT_EQ(retrieved->getCourseId(), courseId);
    EXPECT_FALSE(retrieved->getTitle().empty());
    EXPECT_DOUBLE_EQ(retrieved->getMaxMark(), 100.0);

    // Get by course
    auto assignments = repo.getByCourse(courseId);
    EXPECT_EQ(assignments.size(), 1);
}

TEST_F(DatabaseIntegrationTest, SubmissionRepositoryCrud) {
    LmsTestData::TestData data(*m_db);
    const int departmentId = data.makeDepartment();
    const int courseId = data.makeCourse(departmentId);
    const int lecturerId = data.makeUser(Role::Lecturer, departmentId);
    const int assignmentId = data.makeAssignment(courseId, lecturerId);
    const int studentId = data.makeUser(Role::Student, departmentId);

    SqliteSubmissionRepository repo(*m_db);

    // Create
    Submission submission(0, assignmentId, studentId, SubmissionState::Submitted);
    submission.setFirstSubmittedAt("2024-12-30T10:00:00Z");
    int id = repo.save(submission);
    EXPECT_GT(id, 0);

    // Read
    auto retrieved = repo.getById(id);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getId(), id);
    EXPECT_EQ(retrieved->getAssignmentId(), assignmentId);
    EXPECT_EQ(retrieved->getStudentId(), studentId);
    EXPECT_EQ(retrieved->getState(), SubmissionState::Submitted);

    // Get by assignment
    auto submissions = repo.getByAssignment(assignmentId);
    EXPECT_EQ(submissions.size(), 1);

    // Get by state
    auto markedSubmissions = repo.getByState(SubmissionState::Submitted);
    EXPECT_EQ(markedSubmissions.size(), 1);
}

TEST_F(DatabaseIntegrationTest, MarkAuditRepositoryAppendOnly) {
    LmsTestData::TestData data(*m_db);
    const int departmentId = data.makeDepartment();
    const int courseId = data.makeCourse(departmentId);
    const int lecturerId = data.makeUser(Role::Lecturer, departmentId);
    const int assignmentId = data.makeAssignment(courseId, lecturerId);
    const int studentId = data.makeUser(Role::Student, departmentId);
    const int submissionId = data.makeSubmission(assignmentId, studentId);
    const int markerId = data.makeUser(Role::Lecturer, departmentId);

    SqliteMarkAuditRepository repo(*m_db);

    // Add audit entry
    constexpr char kFirstAuditTime[] = "2024-12-30T11:00:00Z";
    constexpr char kSecondAuditTime[] = "2024-12-30T12:00:00Z";
    MarkAuditEntry entry(0, submissionId, markerId, kFirstAuditTime, 0.0, 85.0, "Initial marking");
    repo.add(entry);

    // Retrieve
    auto entries = repo.getBySubmission(submissionId);
    EXPECT_EQ(entries.size(), 1);
    EXPECT_EQ(entries[0].getOldValue(), 0.0);
    EXPECT_EQ(entries[0].getNewValue(), 85.0);
    EXPECT_EQ(entries[0].getReason(), "Initial marking");

    // Add another entry
    MarkAuditEntry entry2(0, submissionId, markerId, kSecondAuditTime, 85.0, 90.0, "Correction");
    repo.add(entry2);

    entries = repo.getBySubmission(submissionId);
    EXPECT_EQ(entries.size(), 2);
}
