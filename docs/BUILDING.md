# Building and testing

This project uses CMake and C++17. The commands below start from the repository
root, the directory containing `CMakeLists.txt`.

## Prerequisites

- CMake 3.16 or newer
- A C++17 compiler (GCC 9+, Clang 10+, or Visual Studio 2019 or newer)
- SQLite 3 development files / headers
- Git and an internet connection for the first configure, when CMake fetches
  GoogleTest

On Ubuntu or Debian, install the system packages with:

```sh
sudo apt-get update
sudo apt-get install -y cmake g++ libsqlite3-dev git
```

On Windows, install CMake, Git, SQLite development files, and Visual Studio
with the C++ workload, or use a MinGW-w64 toolchain. Run the commands from a
Developer PowerShell when using the Visual Studio compiler.

## Configure and build

The following commands use a `build` directory in the repository. CMake
downloads GoogleTest during configuration if it is not already available in
its dependency cache.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

For Visual Studio's multi-configuration generator, select the configuration
when building and testing:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

For MinGW Makefiles on Windows, use a MinGW-w64-enabled terminal:

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Use a fresh build directory if changing compilers or CMake generators. A build
directory caches both choices and cannot safely be reused for a different
toolchain.

## Run the tests

Run both unit and SQLite integration suites from any working directory with:

```sh
ctest --test-dir build --output-on-failure
```

CTest runs both registered test targets. Test fixtures use the schema path
configured by CMake, so they do not rely on the current working directory.
Alternatively, invoke a test executable directly:

```sh
./build/tests/lms_unit_tests
./build/tests/lms_integration_tests
```

On Windows, the executable location depends on the generator. For Visual
Studio, use `build/tests/Debug/lms_unit_tests.exe` and
`build/tests/Debug/lms_integration_tests.exe`; for single-configuration
generators, use `build/tests/lms_unit_tests.exe` and
`build/tests/lms_integration_tests.exe`.

## Run the application

```sh
./build/src/lms_app
```

On Windows, run `build/src/Debug/lms_app.exe` with Visual Studio, or
`build/src/lms_app.exe` with a single-configuration generator. Run it from the
repository root so its runtime database and other working-directory-relative
files are placed where expected. The schema and demo seed file locations are
provided to the application by CMake at configure time.

## Common troubleshooting

- **SQLite is not found:** install SQLite development headers and libraries,
  then configure again. If the project is on Windows, make sure the SQLite
  package matches the selected compiler and architecture.
- **GoogleTest download fails:** check network access and rerun the configure
  command. CMake's dependency download happens during configuration.
- **A cached generator or compiler is wrong:** configure into a new build
  directory, for example `build-msvc` or `build-mingw`.
- **Tests fail to find the schema:** use the current CMake targets and rerun
  configuration so the compile-time schema path is refreshed.
