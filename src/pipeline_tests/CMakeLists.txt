file(GLOB pipeline_tests_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_executable(pipeline_tests ${pipeline_tests_sources})
target_link_libraries(
    pipeline_tests
    PRIVATE core dewarping test_support Qt6::Widgets Qt6::Xml Boost::unit_test_framework)
target_compile_definitions(
    pipeline_tests
    PRIVATE GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden"
            ARTIFACT_DIR="${CMAKE_BINARY_DIR}/test-artifacts"
            TEST_TMP_ROOT="${CMAKE_BINARY_DIR}/test-tmp")

# Settings files and temporary directories of the tests live here, inside the build tree.
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test-tmp")

add_test(NAME pipeline_tests COMMAND pipeline_tests --log_level=message)
set_tests_properties(pipeline_tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;LC_ALL=en_US.UTF-8;QT_LOGGING_RULES=qt.svg=false;TMPDIR=${CMAKE_BINARY_DIR}/test-tmp"
    TIMEOUT 3600)
