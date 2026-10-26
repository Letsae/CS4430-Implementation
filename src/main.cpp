#include "presentation/ConsoleUI.h"
#include "data/Database.h"
#include "data/UserRepository.h"
#include "data/DepartmentRepository.h"
#include "data/CourseRepository.h"
#include "data/AssignmentRepository.h"
#include "data/SubmissionRepository.h"
#include "data/SubmissionVersionRepository.h"
#include "data/ExtensionRepository.h"
#include "data/MarkAuditRepository.h"
#include "data/EnrolmentRepository.h"
#include "data/SettingsRepository.h"
#include "data/MenuRepository.h"
#include "data/NotificationRepository.h"
#include "application/AuthService.h"
#include "application/SubmissionService.h"
#include "application/MarkingService.h"
#include "application/ModerationService.h"
#include "application/AdminService.h"
#include "application/StudentService.h"
#include "application/MenuService.h"
#include "application/AssignmentService.h"
#include "application/NotificationService.h"
#include "security/BcryptPasswordHasher.h"
#include "domain/LatePenaltyStrategy.h"
#include "domain/Observer.h"
#include "util/Clock.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>

using namespace Lms;

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n"
              << "Options:\n"
              << "  --db <path>          Database file path (default: lms.db)\n"
              << "  --seed-demo          Load demo data from db/seed_demo.sql\n"
              << "  --help               Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::string dbPath = "lms.db";
    bool seedDemo = false;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--db" && i + 1 < argc) {
            dbPath = argv[++i];
        } else if (arg == "--seed-demo") {
            seedDemo = true;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    try {
        // Initialize database
        Database db(dbPath);

        // Load schema if database is new
        // In a real implementation, we'd check if tables exist
        std::ifstream schemaFile(LMS_SCHEMA_PATH);
        if (schemaFile.is_open()) {
            std::stringstream schema;
            schema << schemaFile.rdbuf();
            db.execute(schema.str());
        } else {
            std::cerr << "Warning: Could not load schema file" << std::endl;
        }

        // Initialize repositories
        SqliteUserRepository userRepo(db);
        SqliteDepartmentRepository departmentRepo(db);
        SqliteCourseRepository courseRepo(db);
        SqliteAssignmentRepository assignmentRepo(db);
        SqliteSubmissionRepository submissionRepo(db);
        SqliteSubmissionVersionRepository versionRepo(db);
        SqliteExtensionRepository extensionRepo(db);
        SqliteMarkAuditRepository auditRepo(db);
        SqliteEnrolmentRepository enrolmentRepo(db, courseRepo, userRepo);
        SqliteSettingsRepository settingsRepo(db);
        SqliteMenuRepository menuRepo(db);
        SqliteNotificationRepository notificationRepo(db);

        // Initialize default settings and menu items
        settingsRepo.initializeDefaults();
        menuRepo.initializeDefaults();

        // Initialize utilities
        SystemClock clock;
        int bcryptCost = std::stoi(settingsRepo.getWithDefault("bcrypt_cost", "12"));
        BcryptPasswordHasher hasher(bcryptCost);
        StandardLatePenalty penaltyStrategy;
        EventPublisher eventPublisher;
        auto notificationService = std::make_shared<NotificationService>(enrolmentRepo, notificationRepo, clock);
        eventPublisher.subscribe(notificationService);

        // Initialize services
        AuthService authService(userRepo, hasher);
        SubmissionService submissionService(submissionRepo, assignmentRepo, extensionRepo, eventPublisher, clock);
        MarkingService markingService(submissionRepo, auditRepo, assignmentRepo, extensionRepo, penaltyStrategy, eventPublisher, clock);
        ModerationService moderationService(submissionRepo, auditRepo, eventPublisher, clock);
        AdminService adminService(userRepo, departmentRepo, courseRepo, enrolmentRepo, hasher, db, eventPublisher);
        StudentService studentService(courseRepo, assignmentRepo, submissionRepo, versionRepo, extensionRepo, departmentRepo, userRepo, clock);
        AssignmentService assignmentService(db, courseRepo, assignmentRepo, eventPublisher, clock);
        MenuService menuService(menuRepo);

        // Load optional seed records from a data file rather than embedding demo rows in application code.
        if (seedDemo) {
            auto departments = departmentRepo.getAll();
            if (departments.empty()) {
                std::ifstream seedFile(LMS_DEMO_SEED_PATH);
                if (!seedFile.is_open()) throw DatabaseException("Could not open configured demo seed file");
                std::unordered_map<std::string, int> departmentIds;
                std::unordered_map<std::string, int> userIds;
                std::unordered_map<std::string, int> courseIds;
                int seedAdministratorId = 0;
                std::vector<std::vector<std::string>> records;
                std::string line;
                while (std::getline(seedFile, line)) {
                    std::vector<std::string> fields;
                    std::stringstream row(line);
                    std::string field;
                    while (std::getline(row, field, '|')) fields.push_back(field);
                    if (!fields.empty()) records.push_back(std::move(fields));
                }
                const auto roleFromSeed = [](const std::string& role) {
                    if (role == "STUDENT") return Role::Student;
                    if (role == "LECTURER") return Role::Lecturer;
                    if (role == "HOD") return Role::HeadOfDepartment;
                    if (role == "ADMIN") return Role::Administrator;
                    throw ValidationException("Unknown role in demo seed data");
                };
                const auto departmentIdFor = [&departmentIds](const std::string& name) {
                    if (name.empty()) return 0;
                    const auto found = departmentIds.find(name);
                    if (found == departmentIds.end()) throw ValidationException("Unknown department in demo seed data");
                    return found->second;
                };
                for (const auto& record : records) {
                    if (record[0] == "D" && record.size() == 2)
                        departmentIds.emplace(record[1], adminService.createDepartment(record[1]));
                }
                for (const auto& record : records) {
                    if (record[0] == "U" && record.size() == 5) {
                        const Role role = roleFromSeed(record[2]);
                        const int id = adminService.createUser(record[1], role, record[4], departmentIdFor(record[3]));
                        userIds.emplace(record[1], id);
                        if (role == Role::Administrator && seedAdministratorId == 0) seedAdministratorId = id;
                    }
                }
                for (const auto& record : records) {
                    if (record[0] == "C" && record.size() == 4)
                        courseIds.emplace(record[1], adminService.createCourse(record[1], record[2], departmentIdFor(record[3])));
                }
                auto seedAdmin = userRepo.getById(seedAdministratorId);
                for (const auto& record : records) {
                    if (record[0] == "L" && record.size() == 3)
                        adminService.assignLecturerToCourse(*seedAdmin, courseIds.at(record[1]), userIds.at(record[2]));
                    else if (record[0] == "E" && record.size() == 3)
                        adminService.enrollStudent(courseIds.at(record[1]), userIds.at(record[2]));
                }
                std::cout << "Demo data loaded successfully." << std::endl;
            } else {
                std::cout << "Demo data already exists." << std::endl;
            }
        }

        // Initialize and run UI
        ConsoleUI ui(authService, submissionService, markingService, moderationService, adminService, studentService, assignmentService, menuService);
        ui.run();

        return 0;
    } catch (const LmsException& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
}
