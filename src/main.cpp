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
#include "data/DemoSeedRepository.h"
#include "application/AuthService.h"
#include "application/SubmissionService.h"
#include "application/MarkingService.h"
#include "application/ModerationService.h"
#include "application/AdminService.h"
#include "application/StudentService.h"
#include "application/MenuService.h"
#include "application/AssignmentService.h"
#include "application/ExtensionService.h"
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
#include <random>

using namespace Lms;

namespace {
std::string makeOneTimeDemoPassword() {
    constexpr char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789";
    constexpr size_t passwordLength = 32;
    std::random_device random;
    std::uniform_int_distribution<size_t> select(0, sizeof(alphabet) - 2);
    std::string password;
    password.reserve(passwordLength);
    for (size_t i = 0; i < passwordLength; ++i) password.push_back(alphabet[select(random)]);
    return password;
}
}

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
        const int bcryptCost = std::stoi(settingsRepo.getWithDefault("bcrypt_cost", "12"));
        const auto passwordMinLength = static_cast<size_t>(std::stoull(
            settingsRepo.getWithDefault("password_min_length", "8")));
        const auto passwordMaxLength = static_cast<size_t>(std::stoull(
            settingsRepo.getWithDefault("password_max_length", "72")));
        BcryptPasswordHasher hasher(bcryptCost, passwordMinLength, passwordMaxLength);
        const double latePenaltyPercent = std::stod(settingsRepo.getWithDefault("late_penalty_percent_per_day", "10"));
        const int latePenaltyDays = std::stoi(settingsRepo.getWithDefault("late_penalty_max_days", "3"));
        const int secondsPerDay = std::stoi(settingsRepo.getWithDefault("seconds_per_day", "86400"));
        StandardLatePenalty penaltyStrategy(latePenaltyPercent, latePenaltyDays, secondsPerDay);
        EventPublisher eventPublisher;
        auto notificationService = std::make_shared<NotificationService>(enrolmentRepo, notificationRepo, clock);
        eventPublisher.subscribe(notificationService);

        // Initialize services
        AuthService authService(userRepo, hasher, settingsRepo, clock);
        SubmissionService submissionService(submissionRepo, assignmentRepo, extensionRepo, eventPublisher, clock);
        MarkingService markingService(db, submissionRepo, auditRepo, assignmentRepo, courseRepo,
                                      versionRepo, userRepo, extensionRepo, penaltyStrategy,
                                      eventPublisher, clock);
        ModerationService moderationService(db, submissionRepo, auditRepo, courseRepo, assignmentRepo,
                                            userRepo, eventPublisher, clock);
        AdminService adminService(userRepo, departmentRepo, courseRepo, enrolmentRepo, hasher, db, eventPublisher, clock);
        StudentService studentService(db, courseRepo, assignmentRepo, submissionRepo, versionRepo,
            extensionRepo, departmentRepo, userRepo, settingsRepo, penaltyStrategy, eventPublisher, clock);
        AssignmentService assignmentService(db, courseRepo, assignmentRepo, eventPublisher, clock);
        ExtensionService extensionService(db, assignmentRepo, courseRepo, enrolmentRepo, extensionRepo,
                                           userRepo, eventPublisher, clock);
        MenuService menuService(menuRepo);

        // Load optional seed records from a data file rather than embedding demo rows in application code.
        if (seedDemo) {
            auto departments = departmentRepo.getAll();
            if (departments.empty()) {
                DemoSeedRepository seedRepository(db);
                seedRepository.loadSqlFile(LMS_DEMO_SEED_PATH);
                std::unordered_map<std::string, int> departmentIds;
                std::unordered_map<std::string, int> userIds;
                std::unordered_map<std::string, int> courseIds;
                int seedAdministratorId = 0;
                const std::string demoPassword = makeOneTimeDemoPassword();
                const auto records = seedRepository.getRecords();
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
                    if (record.kind == "D")
                        departmentIds.emplace(record.first, adminService.createSeedDepartment(record.first));
                }
                for (const auto& record : records) {
                    if (record.kind == "U") {
                        const Role role = roleFromSeed(record.second);
                        const int id = adminService.createSeedUser(record.first, role, demoPassword,
                            departmentIdFor(record.third));
                        userIds.emplace(record.first, id);
                        if (role == Role::Administrator && seedAdministratorId == 0) seedAdministratorId = id;
                    }
                }
                for (const auto& record : records) {
                    if (record.kind == "C")
                        courseIds.emplace(record.first, adminService.createSeedCourse(record.first,
                            record.second, departmentIdFor(record.third)));
                }
                auto seedAdmin = userRepo.getById(seedAdministratorId);
                for (const auto& record : records) {
                    if (record.kind == "L")
                        adminService.assignLecturerToCourse(*seedAdmin, courseIds.at(record.first), userIds.at(record.second));
                    else if (record.kind == "E")
                        adminService.enrollStudent(*seedAdmin, courseIds.at(record.first), userIds.at(record.second));
                }
                settingsRepo.set("force_password_change_user_id", std::to_string(seedAdministratorId));
                std::cout << "Demo data loaded. Temporary shared demo password: " << demoPassword << std::endl;
                std::cout << "The administrator must change this password at first login." << std::endl;
            } else {
                std::cout << "Demo data already exists." << std::endl;
            }
        }

        // Initialize and run UI
        ConsoleUI ui(authService, submissionService, markingService, moderationService, adminService,
                     studentService, assignmentService, extensionService, *notificationService, menuService);
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
