cmake_minimum_required(VERSION 3.16)
project(lms VERSION 1.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Find SQLite3
find_package(SQLite3 REQUIRED)
if(NOT TARGET SQLite3::SQLite3)
    add_library(SQLite3::SQLite3 INTERFACE IMPORTED)
    set_target_properties(SQLite3::SQLite3 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${SQLite3_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${SQLite3_LIBRARIES}"
    )
endif()

# Enable testing
enable_testing()

# Include GoogleTest via FetchContent
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Vendored Openwall bcrypt implementation (wrapper MIT license; bcrypt
# algorithm sources carry their upstream public-domain/CC0 notices).
add_library(lms_bcrypt STATIC
  third_party/libbcrypt/src/bcrypt.c
  third_party/libbcrypt/src/crypt_blowfish.c
  third_party/libbcrypt/src/crypt_gensalt.c
  third_party/libbcrypt/src/wrapper.c
)
target_include_directories(lms_bcrypt PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libbcrypt/include
)

target_include_directories(lms_bcrypt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libbcrypt/src
)
if(WIN32)
  target_link_libraries(lms_bcrypt PUBLIC advapi32)
endif()

# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests)
