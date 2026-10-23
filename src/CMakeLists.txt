include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(foundation)
add_subdirectory(math)
add_subdirectory(imageproc)
add_subdirectory(dewarping)
add_subdirectory(core)
add_subdirectory(app)

if (BUILD_TESTS)
  add_subdirectory(qt_tests)
  add_subdirectory(test_support)
  add_subdirectory(pipeline_tests)
endif()