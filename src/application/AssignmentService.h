#pragma once

#include "data/AssignmentRepository.h"
#include "data/CourseRepository.h"
#include "data/Database.h"
#include "domain/Observer.h"
#include "domain/User.h"
#include "util/Clock.h"

namespace Lms {

struct CreatedAssignment {
    Assignment assignment;
    Course course;
    std::string localDeadline;
};

class AssignmentService {
public:
    AssignmentService(Database& db, ICourseRepository& courses, IAssignmentRepository& assignments,
                      EventPublisher& events, const Clock& clock);
    std::vector<Course> listLecturerCourses(const User& lecturer) const;
    void validateTitle(const std::string& title) const;
    void validateTitleForCourse(int courseId, const std::string& title) const;
    void validateMaximumMark(double maxMark) const;
    double parseMaximumMark(const std::string& input) const;
    void validateDeadline(const std::string& localDeadline) const;
    CreatedAssignment createAssignment(const User& lecturer, int courseId, const std::string& title,
                                       const std::string& localDeadline, double maxMark);
private:
    Database& m_db;
    ICourseRepository& m_courses;
    IAssignmentRepository& m_assignments;
    EventPublisher& m_events;
    const Clock& m_clock;
    static std::string trim(const std::string& value);
    static std::string localToUtc(const std::string& localDeadline);
    void requireLecturer(const User& user) const;
};

} // namespace Lms
