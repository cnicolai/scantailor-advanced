file(GLOB test_support_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(test_support STATIC ${test_support_sources})
target_link_libraries(
    test_support
    PUBLIC ${Qt_Gui_lib} ${Qt_Core_lib}
    PRIVATE TIFF::TIFF PNG::PNG JPEG::JPEG)
target_include_directories(test_support PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# Toolchain facts for MANIFEST.txt (Manifest.cpp).  The golden manifest records them so that a check on a
# different toolchain can warn (D-09).

# Xcode version; empty or an error (only the Command Line Tools selected) becomes "unknown".  Line breaks become
# ", " because a ";" in a compile definition would split it.
execute_process(
    COMMAND xcodebuild -version
    OUTPUT_VARIABLE st_xcode_version
    RESULT_VARIABLE st_xcode_result
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if (NOT st_xcode_result EQUAL 0 OR st_xcode_version STREQUAL "")
  set(st_xcode_version "unknown")
else()
  string(REPLACE "\n" ", " st_xcode_version "${st_xcode_version}")
endif()

# The FP contraction setting is frozen in the top-level CMakeLists.txt (Phase 1, D-03); read it from there
# instead of repeating the value, and record "unset" if it ever disappears.
get_directory_property(st_top_compile_options DIRECTORY "${CMAKE_SOURCE_DIR}" COMPILE_OPTIONS)
string(REGEX MATCH "-ffp-contract=([a-z]+)" st_fp_match "${st_top_compile_options}")
if (st_fp_match)
  set(st_fp_contract "${CMAKE_MATCH_1}")
else()
  set(st_fp_contract "unset")
endif()

target_compile_definitions(
    test_support
    PRIVATE ST_CMAKE_VERSION="${CMAKE_VERSION}"
            ST_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
            ST_XCODE_VERSION="${st_xcode_version}"
            ST_FP_CONTRACT="${st_fp_contract}"
            ST_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
