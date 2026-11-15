set(sources
    main.cpp
    tst_core_qt.cpp
    tst_hidpi_qt.cpp
    tst_mac_qt.cpp
    tst_ux_qt.cpp)

# MainWindow needs the icons and style sheets of the application; resources stay on the executable because a static
# library would drop unreferenced .qrc objects.
set(CMAKE_AUTORCC ON)
list(
    APPEND sources
    "${SCANTAILOR_RESOURCES_DIR}/resources.qrc"
    "${SCANTAILOR_RESOURCES_DIR}/dark_scheme/dark_scheme.qrc"
    "${SCANTAILOR_RESOURCES_DIR}/light_scheme/light_scheme.qrc")

add_executable(qt_tests ${sources})
target_link_libraries(
    qt_tests
    PRIVATE app_gui core dewarping foundation Qt6::Test Qt6::Core Qt6::Gui Qt6::Widgets)
target_include_directories(qt_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/src")
# Settings files and temporary files of the tests live in the build tree, as for pipeline_tests.
# SOURCE_ROOT: the Phase 5 scans (key names, dialog parent flags) read the source tree.
target_compile_definitions(
    qt_tests PRIVATE TEST_TMP_ROOT="${CMAKE_BINARY_DIR}/test-tmp" SOURCE_ROOT="${CMAKE_SOURCE_DIR}")

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test-tmp")

add_test(NAME qt_tests COMMAND qt_tests)
set_tests_properties(qt_tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;LC_ALL=en_US.UTF-8;TMPDIR=${CMAKE_BINARY_DIR}/test-tmp")
