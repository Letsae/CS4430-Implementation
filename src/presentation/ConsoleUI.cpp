#include "ConsoleUI.h"
#include "domain/LmsException.h"
#include "application/StudentService.h"
#include <iostream>
#include <limits>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <windows.h>
#include <charconv>
#include <cctype>

namespace Lms {
namespace {
int readBoundedNumber(const std::string& prompt, int maximum) {
    while (true) {
        std::cout << prompt;
        std::string input;
        if (!std::getline(std::cin, input)) return 0;
        int value = 0;
        const auto parsed = std::from_chars(input.data(), input.data() + input.size(), value);
        if (parsed.ec == std::errc{} && parsed.ptr == input.data() + input.size() && value >= 1 && value <= maximum) return value;
        std::cout << "Invalid choice. Please enter a number from 1 to " << maximum << "." << std::endl;
    }
}

bool confirmAction() {
    while (true) {
        std::cout << "Confirm? (y/n): ";
        std::string input;
        if (!std::getline(std::cin, input)) return false;
        std::transform(input.begin(), input.end(), input.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (input == "y") return true;
        if (input == "n") return false;
        std::cout << "Please enter y or n." << std::endl;
    }
}
}

ConsoleUI::ConsoleUI(AuthService& authService,
                     SubmissionService& submissionService,
                     MarkingService& markingService,
                     ModerationService& moderationService,
                     AdminService& adminService,
                     StudentService& studentService,
                     AssignmentService& assignmentService,
                     MenuService& menuService)
    : m_authService(authService), m_submissionService(submissionService),
      m_markingService(markingService), m_moderationService(moderationService),
      m_adminService(adminService), m_studentService(studentService), m_assignmentService(assignmentService),
      m_menuService(menuService) {
    initializeHandlers();
}

void ConsoleUI::initializeHandlers() {
    // Register action handlers
    m_handlers["VIEW_COURSES"] = [this]() { handleViewCourses(); };
    m_handlers["VIEW_SUBMISSIONS"] = [this]() { handleViewSubmissions(); };
    m_handlers["SUBMIT_ASSIGNMENT"] = [this]() { handleSubmitAssignment(); };
    m_handlers["CHANGE_PASSWORD"] = [this]() { handleChangePassword(); };

    // Placeholder handlers for unimplemented actions
    m_handlers["CREATE_ASSIGNMENT"] = [this]() { handleCreateAssignment(); };
    m_handlers["VIEW_ALL_SUBMISSIONS"] = [this]() {
        std::cout << "View submissions - Not yet implemented" << std::endl;
    };
    m_handlers["MARK_SUBMISSION"] = [this]() {
        std::cout << "Mark submission - Not yet implemented" << std::endl;
    };
    m_handlers["GRANT_EXTENSION"] = [this]() {
        std::cout << "Grant extension - Not yet implemented" << std::endl;
    };
    m_handlers["VIEW_DEPT_COURSES"] = [this]() {
        std::cout << "View courses - Not yet implemented" << std::endl;
    };
    m_handlers["VIEW_MARKED_SUBMISSIONS"] = [this]() {
        std::cout << "View marked submissions - Not yet implemented" << std::endl;
    };
    m_handlers["MODERATE_SUBMISSION"] = [this]() {
        std::cout << "Moderate submission - Not yet implemented" << std::endl;
    };
    m_handlers["RELEASE_MARKS"] = [this]() {
        std::cout << "Release marks - Not yet implemented" << std::endl;
    };
    m_handlers["CREATE_USER"] = [this]() { handleCreateUser(); };
    m_handlers["CREATE_DEPARTMENT"] = [this]() { handleCreateDepartment(); };
    m_handlers["CREATE_COURSE"] = [this]() { handleCreateCourse(); };
    m_handlers["ASSIGN_LECTURER"] = [this]() { handleAssignLecturer(); };
    m_handlers["ENROLL_STUDENT"] = [this]() {
        std::cout << "Enroll student - Not yet implemented" << std::endl;
    };
}

void ConsoleUI::run() {
    clearScreen();
    std::cout << "=== Moderated Learning Management System ===" << std::endl;
    std::cout << std::endl;

    showLogin();

    if (m_authService.getCurrentUser()) {
        showMainMenu();
    }
}

void ConsoleUI::showLogin() {
    while (true) {
        std::cout << "--- Login ---" << std::endl;
        std::string username = getInput("Username: ");
        std::string password = getInput("Password: ");

        try {
            m_authService.login(username, password);
            std::cout << "Login successful!" << std::endl;
            showMessage("Press Enter to continue...");
            return;
        } catch (const AuthenticationException& e) {
            std::cout << "Error: " << e.what() << std::endl;
            showMessage("Press Enter to try again...");
        }
    }
}

void ConsoleUI::showMainMenu() {
    while (m_authService.getCurrentUser()) {
        clearScreen();
        User* user = m_authService.getCurrentUser();
        std::cout << "=== Main Menu ===" << std::endl;
        std::cout << "Logged in as: " << user->getUsername() << " (";

        switch (user->getRole()) {
            case Role::Student: std::cout << "Student"; break;
            case Role::Lecturer: std::cout << "Lecturer"; break;
            case Role::HeadOfDepartment: std::cout << "Head of Department"; break;
            case Role::Administrator: std::cout << "Administrator"; break;
        }
        std::cout << ")" << std::endl;
        std::cout << std::endl;

        // Get menu items from database
        auto menuItems = m_menuService.buildMenu(*user);

        // Display menu items
        for (size_t i = 0; i < menuItems.size(); ++i) {
            std::cout << (i + 1) << ". " << menuItems[i].label << std::endl;
        }

        // Always add logout option
        std::cout << (menuItems.size() + 1) << ". Logout" << std::endl;
        std::cout << std::endl;

        int choice = getIntInput("Enter choice: ");

        try {
            if (choice == static_cast<int>(menuItems.size() + 1)) {
                // Logout
                m_authService.logout();
                std::cout << "Logged out." << std::endl;
                return;
            } else if (choice >= 1 && choice <= static_cast<int>(menuItems.size())) {
                // Execute handler
                const auto& item = menuItems[choice - 1];
                auto it = m_handlers.find(item.actionCode);
                if (it != m_handlers.end()) {
                    it->second();
                } else {
                    std::cerr << "Warning: No handler registered for action: " << item.actionCode << std::endl;
                }
            } else {
                std::cout << "Invalid choice." << std::endl;
            }
        } catch (const LmsException& e) {
            std::cout << "Error: " << e.what() << std::endl;
        }

        showMessage("Press Enter to continue...");
    }
}

void ConsoleUI::showMessage(const std::string& message) {
    std::cout << message;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

std::string ConsoleUI::getInput(const std::string& prompt) {
    std::cout << prompt;
    std::string input;
    std::getline(std::cin, input);
    return input;
}

int ConsoleUI::getIntInput(const std::string& prompt) {
    while (true) {
        std::string input = getInput(prompt);
        try {
            return std::stoi(input);
        } catch (...) {
            std::cout << "Invalid number. Please try again." << std::endl;
        }
    }
}

void ConsoleUI::clearScreen() {
    // Clear screen (platform-specific)
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void ConsoleUI::handleViewCourses() {
    User* user = m_authService.getCurrentUser();
    if (!user || user->getRole() != Role::Student) {
        throw AuthorisationException("Only students can view their courses");
    }

    auto courses = m_studentService.listCourses(user->getId());

    if (courses.empty()) {
        std::cout << "You are not enrolled in any courses." << std::endl;
        return;
    }

    std::cout << "=== My Courses ===" << std::endl;
    for (size_t i = 0; i < courses.size(); ++i) {
        std::cout << (i + 1) << ". " << courses[i].code << " - " << courses[i].title << std::endl;
        std::cout << "   Department: " << courses[i].department << std::endl;
        std::cout << "   Lecturers: ";
        if (courses[i].lecturers.empty()) {
            std::cout << "(none)";
        } else {
            for (size_t j = 0; j < courses[i].lecturers.size(); ++j) {
                if (j > 0) {
                    std::cout << ", ";
                }
                std::cout << courses[i].lecturers[j];
            }
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;

    int courseChoice = getIntInput("Select a course to view assignments (0 to go back): ");
    if (courseChoice <= 0 || courseChoice > static_cast<int>(courses.size())) {
        return;
    }

    const auto& course = courses[courseChoice - 1];
    std::cout << "=== Assignments for " << course.code << " ===" << std::endl;

    if (course.assignments.empty()) {
        std::cout << "No assignments for this course." << std::endl;
        return;
    }

    for (const auto& assignment : course.assignments) {
        std::cout << "- " << assignment.title << std::endl;
        std::cout << "  Deadline: " << assignment.effectiveDeadline;
        if (assignment.hasExtension) {
            std::cout << " (extended)";
        }
        std::cout << std::endl;
        std::cout << "  Max Mark: " << assignment.maxMark << std::endl;
        std::cout << "  State: " << assignment.state << std::endl;
        std::cout << std::endl;
    }
}

void ConsoleUI::handleViewSubmissions() {
    User* user = m_authService.getCurrentUser();
    if (!user || user->getRole() != Role::Student) {
        throw AuthorisationException("Only students can view their submissions");
    }

    auto submissions = m_studentService.listSubmissions(user->getId());

    if (submissions.empty()) {
        std::cout << "You have no submissions." << std::endl;
        return;
    }

    std::cout << "=== My Submissions ===" << std::endl;
    for (size_t i = 0; i < submissions.size(); ++i) {
        const auto& sub = submissions[i];
        std::cout << (i + 1) << ". " << sub.courseCode << " - " << sub.assignmentTitle << std::endl;
        std::cout << "   State: " << sub.state << std::endl;
        std::cout << "   Submitted: " << sub.firstSubmittedAt << std::endl;
        std::cout << "   Version: " << sub.latestVersion << std::endl;

        if (sub.state == "Late") {
            std::cout << "   Days Late: " << sub.daysLate << std::endl;
        }

        if (sub.markReleased) {
            std::cout << "   Mark: " << sub.mark << std::endl;
            std::cout << "   Feedback: " << sub.feedback << std::endl;
        } else {
            std::cout << "   Mark: " << sub.feedback << std::endl;
        }
        std::cout << std::endl;
    }
}

void ConsoleUI::handleSubmitAssignment() {
    User* user = m_authService.getCurrentUser();
    if (!user || user->getRole() != Role::Student) {
        throw AuthorisationException("Only students can submit assignments");
    }

    // Get courses with assignments
    auto courses = m_studentService.listCourses(user->getId());

    if (courses.empty()) {
        std::cout << "You are not enrolled in any courses." << std::endl;
        return;
    }

    // Build a flat list of assignments
    std::vector<std::pair<int, std::string>> assignmentList; // (assignmentId, displayString)
    for (const auto& course : courses) {
        for (const auto& assignment : course.assignments) {
            std::string display = course.code + " - " + assignment.title;
            assignmentList.push_back({assignment.id, display});
        }
    }

    if (assignmentList.empty()) {
        std::cout << "No assignments available for submission." << std::endl;
        return;
    }

    std::cout << "=== Available Assignments ===" << std::endl;
    for (size_t i = 0; i < assignmentList.size(); ++i) {
        std::cout << (i + 1) << ". " << assignmentList[i].second << std::endl;
    }

    int choice = getIntInput("Select assignment to submit (0 to cancel): ");
    if (choice <= 0 || choice > static_cast<int>(assignmentList.size())) {
        return;
    }

    int assignmentId = assignmentList[choice - 1].first;

    std::cout << "Enter submission content (max 20000 characters):" << std::endl;
    std::cout << "Type END on a line by itself to finish." << std::endl;
    std::cout << "---" << std::endl;

    std::string content;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "END") {
            break;
        }
        if (!content.empty()) {
            content += "\n";
        }
        content += line;
    }

    try {
        m_studentService.submit(user->getId(), assignmentId, content);
        std::cout << "Assignment submitted successfully!" << std::endl;
    } catch (const LmsException& e) {
        std::cout << "Submission failed: " << e.what() << std::endl;
    }
}

void ConsoleUI::handleCreateAssignment() {
    try {
        User* user = m_authService.getCurrentUser();
        if (!user) throw AuthorisationException("Not logged in");
        const auto courses = m_assignmentService.listLecturerCourses(*user);
        if (courses.empty()) {
            std::cout << "You are not assigned to any courses." << std::endl;
            return;
        }

        std::cout << "=== My Courses ===" << std::endl;
        for (size_t i = 0; i < courses.size(); ++i)
            std::cout << (i + 1) << ". " << courses[i].getCode() << " - " << courses[i].getTitle() << std::endl;

        int courseChoice = 0;
        while (true) {
            const auto input = getInput("Course (pick a number): ");
            try {
                size_t consumed = 0;
                int parsed = std::stoi(input, &consumed);
                if (consumed != input.size() || parsed < 1 || parsed > static_cast<int>(courses.size()))
                    throw std::invalid_argument("course choice");
                courseChoice = parsed;
                break;
            } catch (...) {
                std::cout << "Invalid course selection. Please choose a listed number." << std::endl;
            }
        }
        const Course& course = courses[static_cast<size_t>(courseChoice - 1)];

        std::string title;
        while (true) {
            title = getInput("Title: ");
            try {
                m_assignmentService.validateTitleForCourse(course.getId(), title);
                break;
            } catch (const ValidationException& e) {
                std::cout << e.what() << std::endl;
            }
        }

        std::string deadline;
        while (true) {
            deadline = getInput("Deadline (YYYY-MM-DD HH:MM, school local time): ");
            try {
                m_assignmentService.validateDeadline(deadline);
                break;
            } catch (const ValidationException& e) {
                std::cout << e.what() << std::endl;
            }
        }

        double maxMark = 0.0;
        while (true) {
            const auto input = getInput("Maximum mark: ");
            try {
                maxMark = m_assignmentService.parseMaximumMark(input);
                break;
            } catch (const ValidationException& e) {
                std::cout << e.what() << std::endl;
            } catch (...) {
                std::cout << "Maximum mark must be a number." << std::endl;
            }
        }

        std::string displayTitle = title;
        const auto first = displayTitle.find_first_not_of(" \t\r\n");
        const auto last = displayTitle.find_last_not_of(" \t\r\n");
        displayTitle = first == std::string::npos ? std::string() : displayTitle.substr(first, last - first + 1);
        std::cout << "=== Assignment Summary ===" << std::endl;
        std::cout << "Course: " << course.getCode() << " - " << course.getTitle() << std::endl;
        std::cout << "Title: " << displayTitle << std::endl;
        std::cout << "Deadline: " << deadline << std::endl;
        std::ostringstream formattedMark;
        formattedMark << std::fixed << std::setprecision(2) << maxMark;
        std::cout << "Maximum mark: " << formattedMark.str() << std::endl;

        std::string confirmation;
        while (true) {
            confirmation = getInput("Create this assignment? (y/n): ");
            std::transform(confirmation.begin(), confirmation.end(), confirmation.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (confirmation == "y" || confirmation == "n") break;
            std::cout << "Please enter y or n." << std::endl;
        }
        if (confirmation == "n") return;

        const auto created = m_assignmentService.createAssignment(*user, course.getId(), title, deadline, maxMark);
        std::cout << "Assignment created successfully. ID: " << created.assignment.getId() << std::endl;
        std::cout << "Course: " << created.course.getCode() << " - " << created.course.getTitle() << std::endl;
        std::cout << "Title: " << created.assignment.getTitle() << std::endl;
        std::cout << "Deadline: " << created.localDeadline << std::endl;
        std::ostringstream formattedCreatedMark;
        formattedCreatedMark << std::fixed << std::setprecision(2) << created.assignment.getMaxMark();
        std::cout << "Maximum mark: " << formattedCreatedMark.str() << std::endl;
    } catch (const DatabaseException&) {
        std::cout << "Unable to access the database. Please try again." << std::endl;
    } catch (const LmsException& e) {
        std::cout << "Unable to create assignment: " << e.what() << std::endl;
    }
}

void ConsoleUI::handleCreateDepartment() {
    try {
        const auto name = getInput("Department name: ");
        const int id = m_adminService.createDepartment(name);
        std::cout << "Department created successfully. ID: " << id << std::endl;
    } catch (const DatabaseException&) {
        std::cout << "Unable to save the department because of a database error." << std::endl;
    } catch (const LmsException& error) {
        std::cout << "Unable to create department: " << error.what() << std::endl;
    }
}

void ConsoleUI::handleCreateCourse() {
    try {
        const auto departments = m_adminService.listDepartments();
        if (departments.empty()) {
            std::cout << "No departments exist. Create a department first." << std::endl;
            return;
        }
        std::cout << "=== Departments ===" << std::endl;
        for (size_t i = 0; i < departments.size(); ++i) {
            std::cout << (i + 1) << ". " << departments[i].getName() << std::endl;
        }
        const int selected = readBoundedNumber("Select a department: ", static_cast<int>(departments.size()));
        if (selected == 0) return;
        const auto code = getInput("Course code: ");
        const auto title = getInput("Course title: ");
        const int id = m_adminService.createCourse(code, title, departments[selected - 1].getId());
        std::cout << "Course created successfully. ID: " << id << std::endl;
    } catch (const DatabaseException&) {
        std::cout << "Unable to save the course because of a database error." << std::endl;
    } catch (const LmsException& error) {
        std::cout << "Unable to create course: " << error.what() << std::endl;
    }
}

void ConsoleUI::handleCreateUser() {
    try {
        std::cout << "=== User Roles ===" << std::endl
                  << "1. Student" << std::endl << "2. Lecturer" << std::endl
                  << "3. Head of Department" << std::endl << "4. Administrator" << std::endl;
        const int roleChoice = readBoundedNumber("Select a role: ", 4);
        if (roleChoice == 0) return;
        const Role role = roleChoice == 1 ? Role::Student : roleChoice == 2 ? Role::Lecturer
            : roleChoice == 3 ? Role::HeadOfDepartment : Role::Administrator;
        int departmentId = 0;
        if (role == Role::Lecturer || role == Role::HeadOfDepartment) {
            const auto departments = m_adminService.listDepartments();
            if (departments.empty()) {
                std::cout << "No departments exist. Create a department first." << std::endl;
                return;
            }
            std::cout << "=== Departments ===" << std::endl;
            for (size_t i = 0; i < departments.size(); ++i) {
                std::cout << (i + 1) << ". " << departments[i].getName() << std::endl;
            }
            const int selected = readBoundedNumber("Select a department: ", static_cast<int>(departments.size()));
            if (selected == 0) return;
            departmentId = departments[selected - 1].getId();
        }
        const auto username = getInput("Username: ");
        const auto password = getInput("Password: ");
        const int id = m_adminService.createUser(username, role, password, departmentId);
        std::cout << "User created successfully. ID: " << id << std::endl;
    } catch (const DatabaseException&) {
        std::cout << "Unable to save the user because of a database error." << std::endl;
    } catch (const LmsException& error) {
        std::cout << "Unable to create user: " << error.what() << std::endl;
    }
}

void ConsoleUI::handleAssignLecturer() {
    try {
        const auto courses = m_adminService.listCoursesForAssignment();
        if (courses.empty()) {
            std::cout << "No courses exist. Create a course first." << std::endl;
            return;
        }
        std::cout << "=== Courses ===" << std::endl;
        for (size_t i = 0; i < courses.size(); ++i) {
            std::cout << (i + 1) << ". " << courses[i].course.getCode() << " - "
                      << courses[i].course.getTitle() << " (" << courses[i].department << "; lecturers: ";
            if (courses[i].lecturers.empty()) std::cout << "none";
            else for (size_t j = 0; j < courses[i].lecturers.size(); ++j) {
                if (j) std::cout << ", ";
                std::cout << courses[i].lecturers[j];
            }
            std::cout << ")" << std::endl;
        }
        const int courseChoice = readBoundedNumber("Select a course: ", static_cast<int>(courses.size()));
        if (courseChoice == 0) return;

        const auto lecturers = m_adminService.listActiveLecturers();
        if (lecturers.empty()) {
            std::cout << "No lecturers exist. Create a lecturer first." << std::endl;
            return;
        }
        std::cout << "=== Active Lecturers ===" << std::endl;
        for (size_t i = 0; i < lecturers.size(); ++i) {
            const auto* lecturer = dynamic_cast<const Lecturer*>(lecturers[i].get());
            std::string department = "unknown department";
            if (lecturer) {
                for (const auto& item : m_adminService.listDepartments()) {
                    if (item.getId() == lecturer->getDepartmentId()) department = item.getName();
                }
            }
            std::cout << (i + 1) << ". " << lecturers[i]->getUsername() << " - "
                      << lecturers[i]->getUsername() << " (" << department << ")" << std::endl;
        }
        const int lecturerChoice = readBoundedNumber("Select a lecturer: ", static_cast<int>(lecturers.size()));
        if (lecturerChoice == 0) return;
        const auto& course = courses[courseChoice - 1];
        const auto& lecturer = *lecturers[lecturerChoice - 1];
        std::cout << "Assign " << lecturer.getUsername() << " to " << course.course.getCode()
                  << " - " << course.course.getTitle() << "?" << std::endl;
        if (!confirmAction()) return;
        User* admin = m_authService.getCurrentUser();
        if (!admin) throw AuthorisationException("Not logged in");
        m_adminService.assignLecturerToCourse(*admin, course.course.getId(), lecturer.getId());
        std::cout << "Lecturer assigned successfully." << std::endl;
    } catch (const DatabaseException&) {
        std::cout << "Unable to assign the lecturer because of a database error." << std::endl;
    } catch (const LmsException& error) {
        std::cout << "Unable to assign lecturer: " << error.what() << std::endl;
    }
}

void ConsoleUI::handleChangePassword() {
    User* user = m_authService.getCurrentUser();
    if (!user) {
        throw AuthorisationException("Not logged in");
    }

    std::cout << "=== Change Password ===" << std::endl;

    // Get old password (secure input on Windows)
    std::cout << "Current password: ";
    std::string oldPassword;
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hStdin, &mode);
    SetConsoleMode(hStdin, mode & ~ENABLE_ECHO_INPUT);
    std::getline(std::cin, oldPassword);
    SetConsoleMode(hStdin, mode);
    std::cout << std::endl;

    // Get new password
    std::cout << "New password (8-72 characters): ";
    std::string newPassword;
    SetConsoleMode(hStdin, mode & ~ENABLE_ECHO_INPUT);
    std::getline(std::cin, newPassword);
    SetConsoleMode(hStdin, mode);
    std::cout << std::endl;

    // Confirm new password
    std::cout << "Confirm new password: ";
    std::string confirmPassword;
    SetConsoleMode(hStdin, mode & ~ENABLE_ECHO_INPUT);
    std::getline(std::cin, confirmPassword);
    SetConsoleMode(hStdin, mode);
    std::cout << std::endl;

    try {
        m_authService.changePassword(user->getId(), oldPassword, newPassword, confirmPassword);
        std::cout << "Password changed successfully!" << std::endl;
    } catch (const LmsException& e) {
        std::cout << "Password change failed: " << e.what() << std::endl;
    }
}

} // namespace Lms
