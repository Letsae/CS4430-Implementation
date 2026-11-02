# Static analysis and known findings

Analysis status for the current Windows workspace:

| Tool | Result |
|---|---|
| clang-tidy | Not run: executable is not installed or on `PATH` (`where.exe clang-tidy` found no file). |
| cppcheck | Not run: executable is not installed or on `PATH` (`where.exe cppcheck` found no file). |
| gcov | Available at `C:\msys64\ucrt64\bin\gcov.exe`; coverage has not yet been collected. |

The password hasher now uses vendored upstream `libbcrypt` at commit `d6523c370de6e724ce4ec703e2449b5b028ea3b1`. `third_party/libbcrypt/LICENSE` contains its MIT wrapper license; upstream bcrypt algorithm files retain their public-domain/CC0 notices. A small include compatibility adjustment is applied in the upstream Windows bcrypt wrapper so the Windows build sees required declarations. Generated hashes use standard `$2a$` encoding and embedded salts.

Known implementation findings that remain open:

- `src/application/SubmissionService.cpp` is obsolete, contains a TODO, and emits unused-parameter warnings; the console uses `StudentService` for submission operations.
- Full role/action authorization matrix tests, exhaustive deadline boundary tests, extension service unit tests, and admin creation validation tests remain incomplete.
- Some course, user and title maximum lengths remain hard-coded. Submission length, password length, bcrypt cost, lockout attempts/delay, and penalty settings are read from configuration in the production composition root.
- Coverage has not been measured; no 70% claim is made.

Static analysis tools were unavailable, so a clean clang-tidy/cppcheck run is not claimed.
