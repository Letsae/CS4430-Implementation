#pragma once

#include "domain/User.h"
#include "data/MenuRepository.h"
#include "application/AuthService.h"
#include "application/SubmissionService.h"
#include "application/MarkingService.h"
#include "application/ModerationService.h"
#include "application/AdminService.h"
#include "application/StudentService.h"
#include "application/AssignmentService.h"
#include "application/MenuService.h"
#include <memory>
#include <functional>
#include <map>

namespace Lms {

/**
 * Console-based user interface with role-specific menus.
 */
class ConsoleUI {
public:
    ConsoleUI(AuthService& authService,
              SubmissionService& submissionService,
              MarkingService& markingService,
              ModerationService& moderationService,
              AdminService& adminService,
              StudentService& studentService,
              AssignmentService& assignmentService,
              MenuService& menuService);

    /**
     * Run the main application loop.
     */
    void run();

private:
    AuthService& m_authService;
    SubmissionService& m_submissionService;
    MarkingService& m_markingService;
    ModerationService& m_moderationService;
    AdminService& m_adminService;
    StudentService& m_studentService;
    AssignmentService& m_assignmentService;
    MenuService& m_menuService;

    // Action handler registry
    std::map<std::string, std::function<void()>> m_handlers;

    /**
     * Initialize action handlers.
     */
    void initializeHandlers();

    /**
     * Show login screen.
     */
    void showLogin();

    /**
     * Show main menu based on user role (data-driven).
     */
    void showMainMenu();

    /**
     * Handle "View My Courses" for students.
     */
    void handleViewCourses();

    /**
     * Handle "View My Submissions" for students.
     */
    void handleViewSubmissions();

    /**
     * Handle "Submit Assignment" for students.
     */
    void handleSubmitAssignment();

    /** Handle lecturer assignment creation. */
    void handleCreateAssignment();
    void handleCreateUser();
    void handleCreateDepartment();
    void handleCreateCourse();
    void handleAssignLecturer();

    /**
     * Handle "Change Password" for all users.
     */
    void handleChangePassword();

    /**
     * Display a message and wait for user input.
     */
    void showMessage(const std::string& message);

    /**
     * Get user input as string.
     */
    std::string getInput(const std::string& prompt);

    /**
     * Get user input as integer.
     */
    int getIntInput(const std::string& prompt);

    /**
     * Clear the console screen.
     */
    void clearScreen();
};

} // namespace Lms
