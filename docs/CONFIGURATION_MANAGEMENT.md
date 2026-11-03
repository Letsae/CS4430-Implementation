# Configuration Management — Moderated LMS

## 1. Version Control

The project uses Git for source-code version control and GitHub for remote repository hosting.

Repository: https://github.com/Letsae/CS4430-Implementation

## 2. Branching Strategy

The project follows a feature-branch workflow:

- `main`: stable, tested code.
- `feature/<name>`: development of individual features.
- `fix/<name>`: correction of defects.
- `docs/<name>`: documentation changes.

Changes are developed on separate branches and merged into `main` through pull requests after review and testing.

## 3. Commit Strategy

Commits use descriptive messages that explain the purpose of each change.

Examples:
- `feat: implement submission audit logging`
- `fix: correct late penalty calculation`
- `test: add moderation integration tests`
- `docs: add manual testing instructions`

## 4. Continuous Integration

GitHub Actions automatically builds the project and runs automated tests when changes are pushed or pull requests are opened.

The pipeline performs:
1. CMake configuration.
2. Project compilation.
3. Automated GoogleTest execution.
4. Test result reporting.
5. Coverage reporting where configured.

## 5. Release Management

Stable versions are identified using Git tags.

Example release tag: `v1.0.0`

Maintenance releases follow semantic versioning: a new, backwards-compatible capability raises the minor version. Change request CR1 (class-wide extensions) was released as `v1.1.0` from the `v1.0.0` baseline on a `feature/cr1-class-extensions` branch, merged through a pull request after the regression suite passed (`docs/CHANGE_REQUEST_CR1.md`). Suggested commands: `git checkout -b feature/cr1-class-extensions v1.0.0`, commit (`feat: class-wide deadline extensions (CR1)`, `test: add CR1 tests`, `docs: CR1 analysis and v1.1.0 notes`), merge to `main`, then `git tag -a v1.1.0 -m "Release 1.1.0: CR1"`.

A release is created after the project builds successfully and the automated tests pass.

## 6. Configuration Management Evidence

Evidence includes:
- GitHub repository and commit history.
- Feature branches and pull requests.
- GitHub Actions workflow and successful CI runs.
- Release tags.
