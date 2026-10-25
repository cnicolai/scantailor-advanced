set(sources
    main.cpp
    tst_core_qt.cpp
    tst_hidpi_qt.cpp
    tst_ux_qt.cpp)

add_executable(qt_tests ${sources})
target_link_libraries(
    qt_tests
    PRIVATE core foundation Qt6::Test Qt6::Core Qt6::Gui Qt6::Widgets)
target_include_directories(qt_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/src")
# Settings files and temporary files of the tests live in the build tree, as for pipeline_tests.
target_compile_definitions(qt_tests PRIVATE TEST_TMP_ROOT="${CMAKE_BINARY_DIR}/test-tmp")

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test-tmp")

add_test(NAME qt_tests COMMAND qt_tests)
set_tests_properties(qt_tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;LC_ALL=en_US.UTF-8;TMPDIR=${CMAKE_BINARY_DIR}/test-tmp")
