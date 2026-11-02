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
#include <charconv>
#include <cctype>
#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace Lms {
namespace {
std::string submissionStateLabel(SubmissionState state) {
    switch (state) {
        case SubmissionState::NotSubmitted: return "Not Submitted";
        case SubmissionState::Submitted: return "Submitted";
        case SubmissionState::Late: return "Late";
        case SubmissionState::Marked: return "Marked";
        case SubmissionState::Moderated: return "Moderated";
        case SubmissionState::Released: return "Released";
    }
    return "Unknown";
}

int readBoundedNumber(const std::string& prompt, int maximum, bool allowCancel = false) {
    while (true) {
        std::cout << prompt;
        std::string input;
        if (!std::getline(std::cin, input)) return 0;
        int value = 0;
        const auto parsed = std::from_chars(input.data(), input.data() + input.size(), value);
        const int minimum = allowCancel ? 0 : 1;
        if (parsed.ec == std::errc{} && parsed.ptr == input.data() + input.size() && value >= minimum && value <= maximum) return value;
        std::cout << "Invalid choice. Please enter a number from " << minimum << " to " << maximum << "." << std::endl;
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

std::string readPassword(const std::string& prompt) {
    std::cout << prompt;
    std::cout.flush();
    std::string password;
#ifdef _WIN32
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    const bool isConsole = input != INVALID_HANDLE_VALUE && GetConsoleMode(input, &mode) != 0;
    if (isConsole) {
        // Read directly from the Windows console to avoid stdin buffering
        // problems when changing console echo mode.
        for (;;) {
            const int ch = _getch();
            if (ch == '\r' || ch == '\n') break;
            if (ch == 3) { password.clear(); break; }
            if (ch == 8) {
                if (!password.empty()) {
                    password.pop_back();
                    // Erase the displayed asterisk without revealing the character.
                    std::cout << "\b \b";
                    std::cout.flush();
                }
                continue;
            }
            if (ch == 0 || ch == 224) { (void)_getch(); continue; }
            if (ch >= 32 && ch <= 126) {
                password.push_back(static_cast<char>(ch));
                std::cout << '*';
                std::cout.flush();
            }
        }
    } else {
        // Preserve redirected stdin support for automated tests.
        std::getline(std::cin, password);
    }
#else
    termios originalSettings{};
    const bool canHideInput = tcgetattr(STDIN_FILENO, &originalSettings) == 0;
    if (canHideInput) {
        termios hiddenSettings = originalSettings;
        hiddenSettings.c_lflag &= static_cast<tcflag_t>(~ECHO);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &hiddenSettings);
    }
    std::getline(std::cin, password);
    if (canHideInput) tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalSettings);
#endif
    std::cout << std::endl;
    return password;
}
}

ConsoleUI::ConsoleUI(AuthService& authService,
                     SubmissionService& submissionService,
                     MarkingService& markingService,
                     ModerationService& moderationService,
                     AdminService& adminService,
                     StudentService& studentService,
                     AssignmentService& assignmentService,
                     ExtensionService& extensionService,
                     NotificationService& notificationService,
                     MenuService& menuService)
    : m_authService(authService), m_submissionService(submissionService),
      m_markingService(markingService), m_moderationService(moderationService),
      m_adminService(adminService), m_studentService(studentService), m_assignmentService(assignmentService),
      m_extensionService(extensionService), m_notificationService(notificationService),
      m_menuService(menuService) {
    initializeHandlers();
}

void ConsoleUI::initializeHandlers() {
    // Register action handlers
    m_handlers["VIEW_COURSES"] = [this]() { handleViewCourses(); };
    m_handlers["VIEW_SUBMISSIONS"] = [this]() { handleViewSubmissions(); };
    m_handlers["SUBMIT_ASSIGNMENT"] = [this]() { handleSubmitAssignment(); };
    m_handlers["CHANGE_PASSWORD"] = [this]() { handleChangePassword(); };

    // Register handlers for role-specific actions.
    m_handlers["CREATE_ASSIGNMENT"] = [this]() { handleCreateAssignment(); };
    m_handlers["VIEW_ALL_SUBMISSIONS"] = [this]() { handleViewLecturerSubmissions(); };
    m_handlers["MARK_SUBMISSION"] = [this]() { handleMarkSubmission(); };
    m_handlers["GRANT_EXTENSION"] = [this]() { handleGrantExtension(); };
    m_handlers["VIEW_DEPT_COURSES"] = [this]() { handleViewDepartmentCourses(); };
    m_handlers["VIEW_MARKED_SUBMISSIONS"] = [this]() { handleReviewMarks(); };
    m_handlers["MODERATE_SUBMISSION"] = [this]() { handleModerateMark(); };
    m_handlers["RELEASE_MARKS"] = [this]() { handleReleaseMarks(); };
    m_handlers["VIEW_MARK_AUDIT_LOG"] = [this]() { handleViewMarkAudit(); };
    m_handlers["CREATE_USER"] = [this]() { handleCreateUser(); };
    m_handlers["CREATE_DEPARTMENT"] = [this]() { handleCreateDepartment(); };
    m_handlers["CREATE_COURSE"] = [this]() { handleCreateCourse(); };
    m_handlers["ASSIGN_LECTURER"] = [this]() { handleAssignLecturer(); };
    m_handlers["ENROLL_STUDENT"] = [this]() { handleEnrollStudent(); };
}

void ConsoleUI::run() {
    clearScreen();
    std::cout << "=== Moderated Learning Management System ===" << std::endl;
    std::cout << std::endl;

    showLogin();

    if (m_authService.getCurrentUser()) {
        if (m_authService.isPasswordChangeRequired()) {
            std::cout << "A password change is required before continuing." << std::endl;
            while (m_authService.getCurrentUser() && m_authService.isPasswordChangeRequired()) {
                handleChangePassword();
                if (m_authService.isPasswordChangeRequired())
                    showMessage("Password change required. Press Enter to try again...");
            }
        }
        if (m_authService.getCurrentUser()) showMainMenu();
    }
}

void ConsoleUI::showLogin() {
    while (true) {
        std::cout << "--- Login ---" << std::endl;
        std::string username = getInput("Username: ");
        std::string password = readPassword("Password: ");

        try {
            m_authService.login(username, password);
            std::cout << "Login successful!" << std::endl;
            if (User* user = m_authService.getCurrentUser()) {
                try {
                    const auto notifications = m_notificationService.listUnread(*user);
                    for (const auto& notification : notifications) {
                        std::cout << "Notification: " << notification.getMessage() << std::endl;
                        m_notificationService.markRead(*user, notification.getId());
                    }
                } catch (const LmsException&) {
                    std::cout << "Notifications are temporarily unavailable." << std::endl;
                }
            }
            showMessage("Press Enter to continue...");
            return;
        } catch (const AuthenticationException& e) {
            std::cout << "Error: " << e.what() << std::endl;
            showMessage("Press Enter to try again...");
        } catch (const DatabaseException&) {
            std::cout << "Unable to access the database. Please try again." << std::endl;
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
    if (!user) {
        throw AuthorisationException("Not logged in");
    }
    if (user->getRole() == Role::Lecturer) {
        const auto courses = m_assignmentService.listLecturerCourses(*user);
        if (courses.empty()) {
            std::cout << "You are not assigned to any courses." << std::endl;
            return;
        }
        std::cout << "=== My Courses ===" << std::endl;
        for (const auto& course : courses)
            std::cout << course.getCode() << " - " << course.getTitle() << " (enrolled students: "
                      << m_assignmentService.getEnrolledStudentCount(*user, course.getId()) << ")" << std::endl;
        return;
    }
    if (user->getRole() != Role::Student) {
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

void ConsoleUI::handleViewLecturerSubmissions() {
    try {
        User* lecturer = m_authService.getCurrentUser();
        if (!lecturer) throw AuthorisationException("Not logged in");
        const auto assignments = m_markingService.listAssignments(*lecturer);
        if (assignments.empty()) {
            std::cout << "No assignments exist in your courses." << std::endl;
            return;
        }
        for (size_t i = 0; i < assignments.size(); ++i)
            std::cout << (i + 1) << ". " << assignments[i].getTitle() << std::endl;
        const int choice = readBoundedNumber("Select an assignment (0 to cancel): ",
                                            static_cast<int>(assignments.size()), true);
        if (choice == 0) return;
        const auto& assignment = assignments[static_cast<size_t>(choice - 1)];
        const auto submissions = m_markingService.listSubmissions(*lecturer, assignment.getId());
        if (submissions.empty()) {
            std::cout << "No submissions have been received for this assignment." << std::endl;
            return;
        }
        for (const auto& item : submissions) {
            std::cout << item.student << " | " << submissionStateLabel(item.submission.getState())
                      << " | late penalty: " << item.submission.getPenaltyPercent() << "%"
                      << " | latest version: " << (item.latestVersionSubmittedAt.empty() ? "none" : item.latestVersionSubmittedAt);
            if (item.submission.getState() == SubmissionState::Marked ||
                item.submission.getState() == SubmissionState::Moderated ||
                item.submission.getState() == SubmissionState::Released)
                std::cout << " | raw mark: " << item.submission.getRawMark();
            std::cout << std::endl;
        }
    } catch (const DatabaseException&) {
        std::cout << "Unable to access the database. Please try again." << std::endl;
    } catch (const LmsException& error) {
        std::cout << "Unable to view submissions: " << error.what() << std::endl;
    }
}

void ConsoleUI::handleMarkSubmission() {
    try {
        User* lecturer = m_authService.getCurrentUser();
        if (!lecturer) throw AuthorisationException("Not logged in");
        const auto assignments = m_markingService.listAssignments(*lecturer);
        if (assignments.empty()) {
            std::cout << "No assignments exist in your courses." << std::endl;
            return;
        }
        for (size_t i = 0; i < assignments.size(); ++i)
            std::cout << (i + 1) << ". " << assignments[i].getTitle() << std::endl;
        const int assignmentChoice = readBoundedNumber("Select an assignment (0 to cancel): ",
                                                       static_cast<int>(assignments.size()), true);
        if (assignmentChoice == 0) return;
        const auto& assignment = assignments[static_cast<size_t>(assignmentChoice - 1)];
        auto submissions = m_markingService.listSubmissions(*lecturer, assignment.getId());
        submissions.erase(std::remove_if(submissions.begin(), submissions.end(), [](const auto& item) {
            const auto state = item.submission.getState();
            return state != SubmissionState::Submitted && state != SubmissionState::Late && state != SubmissionState::Marked;
        }), submissions.end());
        if (submissions.empty()) {
            std::cout << "No submissions are ready to be marked." << std::endl;
            return;
        }
        for (size_t i = 0; i < submissions.size(); ++i)
            std::cout << (i + 1) << ". " << submissions[i].student << " ("
                      << submissionStateLabel(submissions[i].submission.getState()) << ")" << std::endl;
        const int submissionChoice = readBoundedNumber("Select a submission (0 to cancel): ",
                                                       static_cast<int>(submissions.size()), true);
        if (submissionChoice == 0) return;
        const auto& selected = submissions[static_cast<size_t>(submissionChoice - 1)];
        const std::string markInput = getInput("Raw mark (maximum " + std::to_string(assignment.getMaxMark()) + "): ");
        size_t consumed = 0;
        const double mark = std::stod(markInput, &consumed);
        if (consumed != markInput.size()) throw ValidationException("Mark must be a number");
        const std::string feedback = getInput("Feedback: ");
        std::string reason = MarkingService::InitialMarkReason;
        if (selected.submission.getState() == SubmissionState::Marked)
            reason = getInput("Reason for revision: ");
        m_markingService.markSubmission(*lecturer, selected.submission.getId(), mark, feedback, reason);
        std::cout << "Submission marked successfully." << std::endl;
    } catch (const DatabaseException&) {
        std::cout << "Unable to save the mark because of a database error." << std::endl;
    } catch (const LmsException& error) {
        std::cout << "Unable to mark submission: " << error.what() << std::endl;
    } catch (const std::exception&) {
        std::cout << "Unable to mark submission: enter a valid numeric mark." << std::endl;
    }
}

void ConsoleUI::handleGrantExtension() {
    try {
        User* lecturer = m_authService.getCurrentUser();
        if (!lecturer) throw AuthorisationException("Not logged in");
        const auto assignments = m_extensionService.listAssignments(*lecturer);
        if (assignments.empty()) {
            std::cout << "No assignments exist in your courses." << std::endl;
            return;
        }
        for (size_t i = 0; i < assignments.size(); ++i)
            std::cout << (i + 1) << ". " << assignments[i].getTitle() << std::endl;
        const int assignmentChoice = readBoundedNumber("Select an assignment (0 to cancel): ",
                                                       static_cast<int>(assignments.size()), true);
        if (assignmentChoice == 0) return;
        const auto& assignment = assignments[static_cast<size_t>(assignmentChoice - 1)];
        const auto students = m_extensionService.listEnrolledStudents(*lecturer, assignment.getId());
        if (students.empty()) {
            std::cout << "No active students are enrolled in this course." << std::endl;
            return;
        }
        for (size_t i = 0; i < students.size(); ++i)
            std::cout << (i + 1) << ". " << students[i]->getUsername() << std::endl;
        const int studentChoice = readBoundedNumber("Select a student (0 to cancel): ",
                                                    static_cast<int>(students.size()), true);
        if (studentChoice == 0) return;
        const std::string deadline = getInput("New deadline (ISO 8601 UTC): ");
        const std::string reason = getInput("Reason: ");
        m_extensionService.grantExtension(*lecturer, assignment.getId(),
            students[static_cast<size_t>(studentChoice - 1)]->getId(), deadline, reason);
        std::cout << "Extension granted successfully." << std::endl;
    } catch (const DatabaseException&) {
        std::cout << "Unable to save the extension because of a database error." << std::endl;
    } catch (const LmsException& error) {
        std::cout << "Unable to grant extension: " << error.what() << std::endl;
    }
}

void ConsoleUI::handleCreateDepartment() {
    try {
        User* administrator = m_authService.getCurrentUser();
        if (!administrator) throw AuthorisationException("Not logged in");
        const auto name = getInput("Department name: ");
        const int id = m_adminService.createDepartment(*administrator, name);
        std::cout << "Department created successfully. ID: " << id << std::endl;
    } catch (const DatabaseException&) {
        std::cout << "Unable to save the department because of a database error." << std::endl;
    } catch (const LmsException& error) {
        std::cout << "Unable to create department: " << error.what() << std::endl;
    }
}

void ConsoleUI::handleCreateCourse() {
    try {
        User* administrator = m_authService.getCurrentUser();
        if (!administrator) throw AuthorisationException("Not logged in");
        const auto departments = m_adminService.listDepartments(*administrator);
        if (departments.empty()) {
            std::cout << "No departments exist. Create a department first." << std::endl;
            return;
        }
        std::cout << "=== Departments ===" << std::endl;
        for (size_t i = 0; i < departments.size(); ++i) {
            std::cout << (i + 1) << ". " << departments[i].getName() << std::endl;
        }
        const int selected = readBoundedNumber("Select a department (0 to cancel): ", static_cast<int>(departments.size()), true);
        if (selected == 0) return;
        const auto code = getInput("Course code: ");
        const auto title = getInput("Course title: ");
        const int id = m_adminService.createCourse(*administrator, code, title, departments[selected - 1].getId());
        std::cout << "Course created successfully. ID: " << id << std::endl;
    } catch (const DatabaseException&) {
        std::cout << "Unable to save the course because of a database error." << std::endl;
    } catch (const LmsException& error) {
        std::cout << "Unable to create course: " << error.what() << std::endl;
    }
}

void ConsoleUI::handleCreateUser() {
    try {
        User* administrator = m_authService.getCurrentUser();
        if (!administrator) throw AuthorisationException("Not logged in");
        std::cout << "=== User Roles ===" << std::endl
                  << "1. Student" << std::endl << "2. Lecturer" << std::endl
                  << "3. Head of Department" << std::endl << "4. Administrator" << std::endl;
        const int roleChoice = readBoundedNumber("Select a role (0 to cancel): ", 4, true);
        if (roleChoice == 0) return;
        const Role role = roleChoice == 1 ? Role::Student : roleChoice == 2 ? Role::Lecturer
            : roleChoice == 3 ? Role::HeadOfDepartment : Role::Administrator;
        int departmentId = 0;
        if (role == Role::Lecturer || role == Role::HeadOfDepartment) {
            const auto departments = m_adminService.listDepartments(*administrator);
            if (departments.empty()) {
                std::cout << "No departments exist. Create a department first." << std::endl;
                return;
            }
            std::cout << "=== Departments ===" << std::endl;
            for (size_t i = 0; i < departments.size(); ++i) {
                std::cout << (i + 1) << ". " << departments[i].getName() << std::endl;
            }
            const int selected = readBoundedNumber("Select a department (0 to cancel): ", static_cast<int>(departments.size()), true);
            if (selected == 0) return;
            departmentId = departments[selected - 1].getId();
        }
        const auto username = getInput("Username: ");
        const auto password = readPassword("Password: ");
        const int id = m_adminService.createUser(*administrator, username, role, password, departmentId);
        std::cout << "User created successfully. ID: " << id << std::endl;
    } catch (const DatabaseException&) {
        std::cout << "Unable to save the user because of a database error." << std::endl;
    } catch (const LmsException& error) {
        std::cout << "Unable to create user: " << error.what() << std::endl;
    }
}

void ConsoleUI::handleAssignLecturer() {
    try {
        User* admin = m_authService.getCurrentUser();
        if (!admin) throw AuthorisationException("Not logged in");
        const auto courses = m_adminService.listCoursesForAssignment(*admin);
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
        const int courseChoice = readBoundedNumber("Select a course (0 to cancel): ", static_cast<int>(courses.size()), true);
        if (courseChoice == 0) return;

        const auto lecturers = m_adminService.listActiveLecturers(*admin);
        if (lecturers.empty()) {
            std::cout << "No lecturers exist. Create a lecturer first." << std::endl;
            return;
        }
        std::cout << "=== Active Lecturers ===" << std::endl;
        for (size_t i = 0; i < lecturers.size(); ++i) {
            const auto* lecturer = dynamic_cast<const Lecturer*>(lecturers[i].get());
            std::string department = "unknown department";
            if (lecturer) {
                for (const auto& item : m_adminService.listDepartments(*admin)) {
                    if (item.getId() == lecturer->getDepartmentId()) department = item.getName();
                }
            }
            std::cout << (i + 1) << ". " << lecturers[i]->getUsername()
                      << " (" << department << ")" << std::endl;
        }
        const int lecturerChoice = readBoundedNumber("Select a lecturer (0 to cancel): ", static_cast<int>(lecturers.size()), true);
        if (lecturerChoice == 0) return;
        const auto& course = courses[courseChoice - 1];
        const auto& lecturer = *lecturers[lecturerChoice - 1];
        std::cout << "Assign " << lecturer.getUsername() << " to " << course.course.getCode()
                  << " - " << course.course.getTitle() << "?" << std::endl;
        if (!confirmAction()) return;
        m_adminService.assignLecturerToCourse(*admin, course.course.getId(), lecturer.getId());
        std::cout << "Lecturer assigned successfully." << std::endl;
    } catch (const DatabaseException&) {
        std::cout << "Unable to assign the lecturer because of a database error." << std::endl;
    } catch (const LmsException& error) {
        std::cout << "Unable to assign lecturer: " << error.what() << std::endl;
    }
}

void ConsoleUI::handleEnrollStudent() {
    try {
        User* administrator = m_authService.getCurrentUser();
        if (!administrator) throw AuthorisationException("Not logged in");
        const auto courses = m_adminService.listCoursesForAssignment(*administrator);
        if (courses.empty()) {
            std::cout << "No courses exist. Create a course first." << std::endl;
            return;
        }
        std::cout << "=== Courses ===" << std::endl;
        for (size_t i = 0; i < courses.size(); ++i)
            std::cout << (i + 1) << ". " << courses[i].course.getCode() << " - "
                      << courses[i].course.getTitle() << " (" << courses[i].department << ")" << std::endl;
        const int courseChoice = readBoundedNumber("Select a course (0 to cancel): ", static_cast<int>(courses.size()), true);
        if (courseChoice == 0) return;

        const auto students = m_adminService.listActiveStudents(*administrator);
        if (students.empty()) {
            std::cout << "No active students exist. Create a student first." << std::endl;
            return;
        }
        std::cout << "=== Active Students ===" << std::endl;
        for (size_t i = 0; i < students.size(); ++i)
            std::cout << (i + 1) << ". " << students[i]->getUsername() << std::endl;
        const int studentChoice = readBoundedNumber("Select a student (0 to cancel): ", static_cast<int>(students.size()), true);
        if (studentChoice == 0) return;

        const auto& course = courses[static_cast<size_t>(courseChoice - 1)].course;
        const auto& student = *students[static_cast<size_t>(studentChoice - 1)];
        m_adminService.enrollStudent(*administrator, course.getId(), student.getId());
        std::cout << student.getUsername() << " enrolled in " << course.getCode() << " successfully." << std::endl;
    } catch (const DatabaseException&) {
        std::cout << "Unable to enroll the student because of a database error." << std::endl;
    } catch (const LmsException& error) {
        std::cout << "Unable to enroll student: " << error.what() << std::endl;
    }
}

void ConsoleUI::handleViewDepartmentCourses() {
    try {
        User* hod = m_authService.getCurrentUser();
        if (!hod) throw AuthorisationException("Not logged in");
        const auto courses = m_moderationService.listDepartmentCourses(*hod);
        if (courses.empty()) {
            std::cout << "No courses exist in your department." << std::endl;
            return;
        }
        std::cout << "=== Department Courses ===" << std::endl;
        for (const auto& course : courses)
            std::cout << course.getCode() << " - " << course.getTitle() << std::endl;
    } catch (const DatabaseException&) {
        std::cout << "Unable to access the database. Please try again." << std::endl;
    } catch (const LmsException& error) {
        std::cout << "Unable to view department courses: " << error.what() << std::endl;
    }
}

void ConsoleUI::handleReviewMarks() {
    try {
        User* hod = m_authService.getCurrentUser();
        if (!hod) throw AuthorisationException("Not logged in");
        const auto reviews = m_moderationService.reviewMarkedSubmissions(*hod);
        if (reviews.empty()) {
            std::cout << "No marked submissions are waiting for moderation." << std::endl;
            return;
        }
        std::cout << "=== Marks for Review ===" << std::endl;
        for (const auto& review : reviews) {
            std::cout << review.courseCode << " - " << review.assignmentTitle
                      << "; student: " << review.student << "; lecturer: " << review.lecturer
                      << "; raw: " << review.submission.getRawMark()
                      << "; penalty: " << review.submission.getPenaltyPercent() << "%"
                      << "; final: " << review.submission.getFinalMark()
                      << "; feedback: " << review.submission.getFeedback() << std::endl;
        }
    } catch (const DatabaseException&) {
        std::cout << "Unable to access the database. Please try again." << std::endl;
    } catch (const LmsException& error) {
        std::cout << "Unable to review marks: " << error.what() << std::endl;
    }
}

void ConsoleUI::handleModerateMark() {
    try {
        User* hod = m_authService.getCurrentUser();
        if (!hod) throw AuthorisationException("Not logged in");
        const auto reviews = m_moderationService.reviewMarkedSubmissions(*hod);
        if (reviews.empty()) {
            std::cout << "No marked submissions are waiting for moderation." << std::endl;
            return;
        }
        for (size_t i = 0; i < reviews.size(); ++i)
            std::cout << (i + 1) << ". " << reviews[i].courseCode << " - "
                      << reviews[i].assignmentTitle << " (" << reviews[i].student
                      << ", final " << reviews[i].submission.getFinalMark() << ")" << std::endl;
        const int choice = readBoundedNumber("Select a submission (0 to cancel): ",
                                            static_cast<int>(reviews.size()), true);
        if (choice == 0) return;
        std::string decision;
        do {
            decision = getInput("Approve or adjust mark? (a/u): ");
            std::transform(decision.begin(), decision.end(), decision.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        } while (decision != "a" && decision != "u");
        const std::string reason = getInput("Reason: ");
        const int submissionId = reviews[static_cast<size_t>(choice - 1)].submission.getId();
        if (decision == "a") {
            m_moderationService.approveSubmission(*hod, submissionId, reason);
            std::cout << "Mark approved for release." << std::endl;
            return;
        }
        const std::string value = getInput("Adjusted raw mark: ");
        size_t consumed = 0;
        const double rawMark = std::stod(value, &consumed);
        if (consumed != value.size()) throw ValidationException("Mark must be a number");
        m_moderationService.adjustSubmissionMark(*hod, submissionId, rawMark, reason);
        std::cout << "Mark adjusted and approved for release." << std::endl;
    } catch (const DatabaseException&) {
        std::cout << "Unable to save moderation because of a database error." << std::endl;
    } catch (const LmsException& error) {
        std::cout << "Unable to moderate mark: " << error.what() << std::endl;
    } catch (const std::exception&) {
        std::cout << "Unable to moderate mark: enter a valid numeric mark." << std::endl;
    }
}

void ConsoleUI::handleReleaseMarks() {
    try {
        User* hod = m_authService.getCurrentUser();
        if (!hod) throw AuthorisationException("Not logged in");
        const auto assignments = m_moderationService.listDepartmentAssignments(*hod);
        if (assignments.empty()) {
            std::cout << "No assignments exist in your department." << std::endl;
            return;
        }
        std::cout << "=== Department Assignments ===" << std::endl;
        for (size_t i = 0; i < assignments.size(); ++i)
            std::cout << (i + 1) << ". " << assignments[i].getTitle() << std::endl;
        const int choice = readBoundedNumber("Select an assignment (0 to cancel): ",
                                            static_cast<int>(assignments.size()), true);
        if (choice == 0) return;
        m_moderationService.releaseAssignmentMarks(*hod, assignments[static_cast<size_t>(choice - 1)].getId());
        std::cout << "Moderated marks released successfully." << std::endl;
    } catch (const DatabaseException&) {
        std::cout << "Unable to release marks because of a database error." << std::endl;
    } catch (const LmsException& error) {
        std::cout << "Unable to release marks: " << error.what() << std::endl;
    }
}

void ConsoleUI::handleViewMarkAudit() {
    try {
        User* hod = m_authService.getCurrentUser();
        if (!hod) throw AuthorisationException("Not logged in");
        const auto assignments = m_moderationService.listDepartmentAssignments(*hod);
        if (assignments.empty()) {
            std::cout << "No assignments exist in your department." << std::endl;
            return;
        }
        for (size_t i = 0; i < assignments.size(); ++i)
            std::cout << (i + 1) << ". " << assignments[i].getTitle() << std::endl;
        const int choice = readBoundedNumber("Select an assignment (0 to cancel): ",
                                            static_cast<int>(assignments.size()), true);
        if (choice == 0) return;
        const auto audit = m_moderationService.auditForAssignment(
            *hod, assignments[static_cast<size_t>(choice - 1)].getId());
        if (audit.empty()) {
            std::cout << "No mark changes have been recorded for this assignment." << std::endl;
            return;
        }
        for (const auto& entry : audit)
            std::cout << "submission " << entry.getSubmissionId() << "; user " << entry.getChangedByUsername()
                      << "; " << entry.getChangedAt() << "; " << entry.getOldValue() << " -> "
                      << entry.getNewValue() << "; reason: " << entry.getReason() << std::endl;
    } catch (const DatabaseException&) {
        std::cout << "Unable to access the database. Please try again." << std::endl;
    } catch (const LmsException& error) {
        std::cout << "Unable to view mark audit: " << error.what() << std::endl;
    }
}

void ConsoleUI::handleChangePassword() {
    User* user = m_authService.getCurrentUser();
    if (!user) {
        throw AuthorisationException("Not logged in");
    }

    std::cout << "=== Change Password ===" << std::endl;

    const std::string oldPassword = readPassword("Current password: ");
    const std::string newPassword = readPassword("New password (8-72 characters): ");
    const std::string confirmPassword = readPassword("Confirm new password: ");

    try {
        m_authService.changePassword(user->getId(), oldPassword, newPassword, confirmPassword);
        std::cout << "Password changed successfully!" << std::endl;
    } catch (const LmsException& e) {
        std::cout << "Password change failed: " << e.what() << std::endl;
    }
}

} // namespace Lms
