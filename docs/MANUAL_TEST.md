# Console smoke test

Use a clean database and start the app with `--seed-demo` to load the sample department, users, course, and enrolments from `db/seed_demo.sql`. The app generates and prints one temporary demo password for the sample accounts; the bootstrap Administrator must change it at first login. Use only disposable local data.

1. Sign in as `admin` using the one-time password printed by the seed command. Complete the forced password change. Create a department, then a Head of Department and Lecturer in it.
2. Create a Student in that department, then create a course, assign the Lecturer, and enrol the Student.
3. Sign in as the Lecturer. Open **View My Courses**, create an assignment with a future deadline and maximum mark, then open **View Submissions** to confirm it is initially empty.
4. Sign in as the Student. Open **View My Courses**, submit before the deadline, then submit a second version. Confirm **View My Submissions** reports the latest version and no mark.
5. After the deadline, submit with a second student and confirm the submission is flagged late. Give a third student an extension through the Lecturer workflow and submit after the original deadline but before the extension.
6. Sign in as the Lecturer, mark each submission with feedback, and revise one mark with a reason. Confirm the submissions list shows the states and penalty values.
7. Sign in as the HoD. Review the department's marked work, approve one result and adjust another with a reason. Check the mark audit log.
8. Release the assignment's moderated marks. Sign in as each Student and confirm feedback and marks appear only after release.

The app's current interfaces may not expose every operation yet. For deterministic service coverage, run `ctest --test-dir build --output-on-failure`; the `EndToEndWorkflow` integration test exercises submission, marking, moderation, release and audit persistence on a fresh in-memory SQLite database.

## Demo invocation

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
.\build\src\Debug\lms_app.exe --db demo.db --seed-demo
```

```bash
cmake -S . -B build
cmake --build build
./build/src/lms_app --db demo.db --seed-demo
```
