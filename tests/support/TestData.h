#pragma once

#include "data/Database.h"
#include "domain/Entities.h"
#include "domain/User.h"

namespace LmsTestData {

using namespace Lms;

class TestData {
public:
    explicit TestData(Database& database);

    int makeDepartment();
    int makeUser(Role role, int departmentId = 0);
    int makeCourse(int departmentId);
    void makeEnrolment(int courseId, int studentId);
    int makeAssignment(int courseId, int lecturerId);
    int makeSubmission(int assignmentId, int studentId);

private:
    Database& m_database;
    int nextValue();
};

} // namespace LmsTestData
