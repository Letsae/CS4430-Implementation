# Collect all source files
file(GLOB_RECURSE DOMAIN_SOURCES CONFIGURE_DEPENDS "domain/*.cpp")
file(GLOB_RECURSE APPLICATION_SOURCES CONFIGURE_DEPENDS "application/*.cpp")
file(GLOB_RECURSE DATA_SOURCES CONFIGURE_DEPENDS "data/*.cpp")
file(GLOB_RECURSE SECURITY_SOURCES CONFIGURE_DEPENDS "security/*.cpp")
file(GLOB_RECURSE UTIL_SOURCES CONFIGURE_DEPENDS "util/*.cpp")
file(GLOB_RECURSE PRESENTATION_SOURCES CONFIGURE_DEPENDS "presentation/*.cpp")

# Create lms_core static library (all non-UI code)
add_library(lms_core STATIC
    ${DOMAIN_SOURCES}
    ${APPLICATION_SOURCES}
    ${DATA_SOURCES}
    ${SECURITY_SOURCES}
    ${UTIL_SOURCES}
)

target_include_directories(lms_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lms_core PUBLIC
    SQLite::SQLite3
)

# Create lms_app executable
add_executable(lms_app
    ${PRESENTATION_SOURCES}
    main.cpp
)

file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/db/schema.sql" LMS_SCHEMA_PATH_VALUE)
target_compile_definitions(lms_app PRIVATE
    LMS_SCHEMA_PATH="${LMS_SCHEMA_PATH_VALUE}"
)
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/db/seed_demo.csv" LMS_DEMO_SEED_PATH_VALUE)
target_compile_definitions(lms_app PRIVATE
    LMS_DEMO_SEED_PATH="${LMS_DEMO_SEED_PATH_VALUE}"
)

target_link_libraries(lms_app
    PRIVATE
    lms_core
)
