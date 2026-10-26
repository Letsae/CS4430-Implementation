#include <gtest/gtest.h>
#include "domain/User.h"

using namespace Lms;

TEST(UserFactory, CreatesStudent) {
    auto user = UserFactory::create(Role::Student, 1, "student1");
    ASSERT_NE(user, nullptr);
    EXPECT_EQ(user->getRole(), Role::Student);
    EXPECT_EQ(user->getId(), 1);
    EXPECT_EQ(user->getUsername(), "student1");
}

TEST(UserFactory, CreatesLecturer) {
    auto user = UserFactory::create(Role::Lecturer, 2, "lecturer1", 10);
    ASSERT_NE(user, nullptr);
    EXPECT_EQ(user->getRole(), Role::Lecturer);
    EXPECT_EQ(user->getId(), 2);
    EXPECT_EQ(user->getUsername(), "lecturer1");
}

TEST(UserFactory, CreatesHeadOfDepartment) {
    auto user = UserFactory::create(Role::HeadOfDepartment, 3, "hod1", 10);
    ASSERT_NE(user, nullptr);
    EXPECT_EQ(user->getRole(), Role::HeadOfDepartment);
    EXPECT_EQ(user->getId(), 3);
    EXPECT_EQ(user->getUsername(), "hod1");
}

TEST(UserFactory, CreatesAdministrator) {
    auto user = UserFactory::create(Role::Administrator, 4, "admin1");
    ASSERT_NE(user, nullptr);
    EXPECT_EQ(user->getRole(), Role::Administrator);
    EXPECT_EQ(user->getId(), 4);
    EXPECT_EQ(user->getUsername(), "admin1");
}

TEST(StudentPermissions, CanLogin) {
    Student student(1, "student1");
    EXPECT_TRUE(student.canPerform(Action::Login));
}

TEST(StudentPermissions, CanSubmitAssignment) {
    Student student(1, "student1");
    EXPECT_TRUE(student.canPerform(Action::SubmitAssignment));
}

TEST(StudentPermissions, CannotCreateAssignment) {
    Student student(1, "student1");
    EXPECT_FALSE(student.canPerform(Action::CreateAssignment));
}

TEST(StudentPermissions, CannotMarkSubmission) {
    Student student(1, "student1");
    EXPECT_FALSE(student.canPerform(Action::MarkSubmission));
}

TEST(LecturerPermissions, CanCreateAssignment) {
    Lecturer lecturer(1, "lecturer1", 10);
    EXPECT_TRUE(lecturer.canPerform(Action::CreateAssignment));
}

TEST(LecturerPermissions, CanGrantExtension) {
    Lecturer lecturer(1, "lecturer1", 10);
    EXPECT_TRUE(lecturer.canPerform(Action::GrantExtension));
}

TEST(LecturerPermissions, CanMarkSubmission) {
    Lecturer lecturer(1, "lecturer1", 10);
    EXPECT_TRUE(lecturer.canPerform(Action::MarkSubmission));
}

TEST(LecturerPermissions, CannotModerateSubmission) {
    Lecturer lecturer(1, "lecturer1", 10);
    EXPECT_FALSE(lecturer.canPerform(Action::ModerateSubmission));
}

TEST(HeadOfDepartmentPermissions, CanModerateSubmission) {
    HeadOfDepartment hod(1, "hod1", 10);
    EXPECT_TRUE(hod.canPerform(Action::ModerateSubmission));
}

TEST(HeadOfDepartmentPermissions, CanReleaseMarks) {
    HeadOfDepartment hod(1, "hod1", 10);
    EXPECT_TRUE(hod.canPerform(Action::ReleaseMarks));
}

TEST(HeadOfDepartmentPermissions, CannotCreateAssignment) {
    HeadOfDepartment hod(1, "hod1", 10);
    EXPECT_FALSE(hod.canPerform(Action::CreateAssignment));
}

TEST(AdministratorPermissions, CanCreateUser) {
    Administrator admin(1, "admin1");
    EXPECT_TRUE(admin.canPerform(Action::CreateUser));
}

TEST(AdministratorPermissions, CanCreateDepartment) {
    Administrator admin(1, "admin1");
    EXPECT_TRUE(admin.canPerform(Action::CreateDepartment));
}

TEST(AdministratorPermissions, CanAssignLecturer) {
    Administrator admin(1, "admin1");
    EXPECT_TRUE(admin.canPerform(Action::AssignLecturer));
}

TEST(AdministratorPermissions, CannotMarkSubmission) {
    Administrator admin(1, "admin1");
    EXPECT_FALSE(admin.canPerform(Action::MarkSubmission));
}

TEST(Lecturer, GetDepartmentId) {
    Lecturer lecturer(1, "lecturer1", 42);
    EXPECT_EQ(lecturer.getDepartmentId(), 42);
}

TEST(HeadOfDepartment, GetDepartmentId) {
    HeadOfDepartment hod(1, "hod1", 99);
    EXPECT_EQ(hod.getDepartmentId(), 99);
}
