# Collect test sources
file(GLOB_RECURSE UNIT_TEST_SOURCES CONFIGURE_DEPENDS "unit/*.cpp")
file(GLOB_RECURSE INTEGRATION_TEST_SOURCES CONFIGURE_DEPENDS "integration/*.cpp")
file(GLOB_RECURSE TEST_SUPPORT_SOURCES CONFIGURE_DEPENDS "support/*.cpp")

# Unit tests executable
add_executable(lms_unit_tests ${UNIT_TEST_SOURCES})
target_link_libraries(lms_unit_tests
    PRIVATE
    lms_core
    gtest
    gtest_main
)
target_include_directories(lms_unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/db/schema.sql" LMS_SCHEMA_PATH_VALUE)
target_compile_definitions(lms_unit_tests PRIVATE
    LMS_SCHEMA_PATH="${LMS_SCHEMA_PATH_VALUE}"
)

# Integration tests executable
add_executable(lms_integration_tests ${INTEGRATION_TEST_SOURCES} ${TEST_SUPPORT_SOURCES})
target_link_libraries(lms_integration_tests
    PRIVATE
    lms_core
    gtest
    gtest_main
)
target_include_directories(lms_integration_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)
target_compile_definitions(lms_integration_tests PRIVATE
    LMS_SCHEMA_PATH="${LMS_SCHEMA_PATH_VALUE}"
)

# Register tests
add_test(NAME UnitTests COMMAND lms_unit_tests)
add_test(NAME IntegrationTests COMMAND lms_integration_tests)
set_tests_properties(UnitTests IntegrationTests PROPERTIES
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
