#include "AssignmentService.h"
#include "domain/LmsException.h"
#include "util/Clock.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace Lms {

AssignmentService::AssignmentService(Database& db, ICourseRepository& courses, IAssignmentRepository& assignments,
        EventPublisher& events, const Clock& clock)
    : m_db(db), m_courses(courses), m_assignments(assignments), m_events(events), m_clock(clock) {}

void AssignmentService::requireLecturer(const User& user) const {
    if (user.getRole() != Role::Lecturer || !user.canPerform(Action::CreateAssignment))
        throw AuthorisationException("Only lecturers can create assignments");
}

std::vector<Course> AssignmentService::listLecturerCourses(const User& lecturer) const {
    if (lecturer.getRole() != Role::Lecturer || !lecturer.canPerform(Action::ViewOwnCourses))
        throw AuthorisationException("Only lecturers can view their courses");
    return m_courses.getByLecturer(lecturer.getId());
}

std::string AssignmentService::trim(const std::string& value) {
    auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c); });
    auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c); }).base();
    return first < last ? std::string(first, last) : std::string();
}

void AssignmentService::validateTitle(const std::string& value) const {
    const auto title = trim(value);
    if (title.empty()) throw ValidationException("Title cannot be blank");
    const size_t characters = static_cast<size_t>(std::count_if(title.begin(), title.end(), [](unsigned char c) {
        return (c & 0xC0) != 0x80;
    }));
    if (characters > 100) throw ValidationException("Title must be 100 characters or fewer");
}

void AssignmentService::validateMaximumMark(double maxMark) const {
    if (!std::isfinite(maxMark) || maxMark <= 0.0 || maxMark > 1000.0)
        throw ValidationException("Maximum mark must be greater than 0 and at most 1000");
    if (std::abs(maxMark * 100.0 - std::round(maxMark * 100.0)) > 1e-7)
        throw ValidationException("Maximum mark may have at most 2 decimal places");
}

double AssignmentService::parseMaximumMark(const std::string& input) const {
    const std::string value = trim(input);
    if (value.empty()) throw ValidationException("Maximum mark must be a number");
    size_t position = (value[0] == '+' || value[0] == '-') ? 1 : 0;
    bool digit = false;
    bool decimal = false;
    size_t decimalPlaces = 0;
    for (; position < value.size(); ++position) {
        const unsigned char c = static_cast<unsigned char>(value[position]);
        if (std::isdigit(c)) {
            digit = true;
            if (decimal) ++decimalPlaces;
        } else if (value[position] == '.' && !decimal) {
            decimal = true;
        } else {
            throw ValidationException("Maximum mark must be a number with up to 2 decimal places");
        }
    }
    if (!digit || (decimal && decimalPlaces == 0) || decimalPlaces > 2)
        throw ValidationException("Maximum mark must be a number with up to 2 decimal places");
    size_t consumed = 0;
    double parsed;
    try {
        parsed = std::stod(value, &consumed);
    } catch (const std::exception&) {
        throw ValidationException("Maximum mark must be a finite number");
    }
    if (consumed != value.size()) throw ValidationException("Maximum mark must be a number");
    validateMaximumMark(parsed);
    return parsed;
}

std::string AssignmentService::localToUtc(const std::string& value) {
    if (value.size() != 16 || value[4] != '-' || value[7] != '-' || value[10] != ' ' || value[13] != ':')
        throw ValidationException("Deadline must use YYYY-MM-DD HH:MM format");
    for (size_t i = 0; i < value.size(); ++i) {
        if (i == 4 || i == 7 || i == 10 || i == 13) continue;
        if (!std::isdigit(static_cast<unsigned char>(value[i])))
            throw ValidationException("Deadline must use YYYY-MM-DD HH:MM format");
    }
    std::tm parsed{};
    std::istringstream input(value);
    input >> std::get_time(&parsed, "%Y-%m-%d %H:%M");
    if (input.fail()) throw ValidationException("Deadline must use YYYY-MM-DD HH:MM format");
    parsed.tm_isdst = -1;
    const std::tm requested = parsed;
    const std::time_t localTime = std::mktime(&parsed);
    if (localTime == static_cast<std::time_t>(-1)) throw ValidationException("Deadline is not a valid local date and time");
    std::tm* roundTrip = std::localtime(&localTime);
    if (!roundTrip || roundTrip->tm_year != requested.tm_year || roundTrip->tm_mon != requested.tm_mon ||
        roundTrip->tm_mday != requested.tm_mday || roundTrip->tm_hour != requested.tm_hour || roundTrip->tm_min != requested.tm_min)
        throw ValidationException("Deadline is not a valid local date and time");
    return TimeUtils::toIsoString(std::chrono::system_clock::from_time_t(localTime));
}

void AssignmentService::validateDeadline(const std::string& localDeadline) const {
    const auto utc = localToUtc(localDeadline);
    if (TimeUtils::fromIsoString(utc) <= m_clock.nowTimePoint())
        throw ValidationException("Deadline must be later than now");
}

void AssignmentService::validateTitleForCourse(int courseId, const std::string& value) const {
    validateTitle(value);
    const auto normalized = trim(value);
    std::string folded = normalized;
    std::transform(folded.begin(), folded.end(), folded.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (const auto& assignment : m_assignments.getByCourse(courseId)) {
        std::string other = trim(assignment.getTitle());
        std::transform(other.begin(), other.end(), other.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (other == folded) throw ValidationException("An assignment with this title already exists for the course");
    }
}

CreatedAssignment AssignmentService::createAssignment(const User& lecturer, int courseId, const std::string& rawTitle,
        const std::string& localDeadline, double maxMark) {
    requireLecturer(lecturer);
    const std::string title = trim(rawTitle);
    validateTitle(title);
    validateMaximumMark(maxMark);
    const std::string deadlineUtc = localToUtc(localDeadline);
    if (TimeUtils::fromIsoString(deadlineUtc) <= m_clock.nowTimePoint())
        throw ValidationException("Deadline must be later than now");

    auto course = m_courses.getById(courseId);
    const auto taughtCourses = m_courses.getByLecturer(lecturer.getId());
    if (std::none_of(taughtCourses.begin(), taughtCourses.end(), [courseId](const Course& c) { return c.getId() == courseId; }))
        throw AuthorisationException("You are not assigned to this course");

    const auto existing = m_assignments.getByCourse(courseId);
    std::string foldedTitle = title;
    std::transform(foldedTitle.begin(), foldedTitle.end(), foldedTitle.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (const auto& assignment : existing) {
        std::string other = trim(assignment.getTitle());
        std::transform(other.begin(), other.end(), other.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (other == foldedTitle) throw ValidationException("An assignment with this title already exists for the course");
    }

    Assignment assignment(0, courseId, title, deadlineUtc, maxMark, lecturer.getId());
    TransactionGuard transaction(m_db);
    assignment.setId(m_assignments.save(assignment));
    // Notifications are synchronous observers. Publishing before commit keeps a failed notification
    // insert in the same SQLite transaction and rolls back the assignment as well.
    AssignmentCreatedEvent event(assignment.getId(), courseId, title, localDeadline);
    m_events.publish(event);
    transaction.commit();
    return {assignment, *course, localDeadline};
}

} // namespace Lms
