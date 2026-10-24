cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

set(target_name scantailor-advanced)

# Disable in-source builds.
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR
      "FATAL: In-source builds are not allowed."
      " You should create a separate directory for build files.")
endif()

# Set cmake policies.
if (POLICY CMP0071)
  cmake_policy(SET CMP0071 NEW)
endif()

if (POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

project("ScanTailor Advanced")

# Set compiler flags.
set(CMAKE_CXX_STANDARD 17)

if (NOT DEFAULT_COMPILER_FLAGS_IS_SET)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}"
      CACHE STRING "Common C flags for all build configurations." FORCE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}"
      CACHE STRING "Common C++ flags for all build configurations." FORCE)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}"
      CACHE STRING "Common link flags for all build configurations." FORCE)

  set(DEFAULT_COMPILER_FLAGS_IS_SET YES CACHE INTERNAL "" FORCE)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # Clang counterpart of the GNU warning set above (-Wpedantic left out, D-08).
  # Outside the cache guard so that every configure applies it.
  add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:-Wall;-Wextra;-Werror=return-type;-Wno-unused>")
  # FP contraction frozen so results do not depend on build type or code layout; fixed before Phase 2 baselines (D-03).
  add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:-ffp-contract=off>")
endif()

set(SANITIZE "" CACHE STRING "Value for -fsanitize=, e.g. address,undefined. Empty disables.")
if (SANITIZE)
  add_compile_options(-fsanitize=${SANITIZE} -fno-omit-frame-pointer)
  add_link_options(-fsanitize=${SANITIZE})
  if (SANITIZE MATCHES "address")
    # Keep running after a finding so one run collects every report.
    add_compile_options(-fsanitize-recover=address)
  endif()
endif()

option(CMAKE_POSITION_INDEPENDENT_CODE "" ON)

include(cmake/SetDefaultBuildType.cmake)
include(cmake/UpdateTranslations.cmake)
include(cmake/ListItemsPrepend.cmake)

st_set_default_build_type(Release)

# Must be set before Boost: the tests need its unit_test_framework component.
option(BUILD_TESTS "Build the unit and pipeline tests" ON)

if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()

#=================================== JPEG ===================================#

find_package(JPEG REQUIRED)

#=================================== ZLIB ===================================#

find_package(ZLIB REQUIRED)

#================================== PNG ==================================#

find_package(PNG REQUIRED)

#=================================== TIFF ===================================#

find_package(TIFF REQUIRED)

#================================= Boost ================================#

#set(Boost_NO_WARN_NEW_VERSIONS ON)

set(Boost_USE_MULTITHREADED ON)

if (BUILD_TESTS)
  find_package(Boost 1.60 COMPONENTS unit_test_framework REQUIRED)
else()
  find_package(Boost 1.60 REQUIRED)
endif()

include_directories(${Boost_INCLUDE_DIRS})
add_definitions(-DBOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
if (BUILD_TESTS)
  add_definitions(-DBOOST_TEST_DYN_LINK)
endif()

#=================================== Qt ===================================#

find_package(Qt6 6.5 COMPONENTS Core Gui Widgets Xml Network LinguistTools OpenGL Svg OpenGLWidgets Test CONFIG REQUIRED)

#=================================== Threads ===================================#

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

#=================================== Main ===================================#

set(CMAKE_AUTOMOC ON)

if (BUILD_TESTS)
  enable_testing()
endif()

# Prepare config.h
set(APPLICATION_NAME "${target_name}")
set(TRANSLATION_DIRS ".:translations:../share/${APPLICATION_NAME}/translations")
set(PLUGIN_DIRS ".:../lib/${APPLICATION_NAME}")
configure_file(config.h.in "${CMAKE_BINARY_DIR}/config.h" @ONLY)
file(READ "${CMAKE_BINARY_DIR}/config.h" config_h_contents)
set(enable_developer_version_default OFF)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(enable_developer_version_default ON)
endif()
option(DEVELOPER_VERSION "Whether to enable the internal debug features." ${enable_developer_version_default})
if (NOT DEVELOPER_VERSION)
  string(REGEX REPLACE "(#define[ \\t]+ENABLE_DEBUG_FEATURES[^\n]*)" "//\\1" config_h_contents ${config_h_contents})
endif()
file(WRITE "${CMAKE_BINARY_DIR}/config.h" ${config_h_contents})
include_directories(BEFORE "${CMAKE_BINARY_DIR}")

# Extract VERSION and VERSION_QUAD from version.h
file(READ "${PROJECT_SOURCE_DIR}/version.h.in" version_h_contents)
string(
    REGEX REPLACE
    ".*#define[ \\t]+VERSION[ \\t]+\"([^\"]*)\".*"
    "\\1" VERSION "${version_h_contents}")
if ("${VERSION}" STREQUAL "${version_h_contents}")
  message(FATAL_ERROR "Failed to extract VERSION from version.h")
endif()
# VERSION_QUAD must be either empty or be in the form of X.Y.Z.Y
string(
    REGEX REPLACE
    ".*#define[ \\t]+VERSION_QUAD[ \\t]+\"(([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)?)\".*"
    "\\1" VERSION_QUAD "${version_h_contents}")
if ("${VERSION_QUAD}" STREQUAL "${version_h_contents}")
  message(FATAL_ERROR "Failed to extract VERSION_QUAD from version.h")
endif()
configure_file(version.h.in "${CMAKE_BINARY_DIR}/version.h" COPYONLY)

set(SCANTAILOR_RESOURCES_DIR "${CMAKE_SOURCE_DIR}/src/resources")
set(SCANTAILOR_TRANSLATIONS_DIR "${CMAKE_SOURCE_DIR}/src/translations")

add_subdirectory(src)

# Translations
file(GLOB translation_files "${SCANTAILOR_TRANSLATIONS_DIR}/${target_name}_*.ts")

finalize_translations(scantailor ${translation_files})
update_translations_target(update_translations scantailor)

list(FILTER translation_files EXCLUDE REGEX ".*untranslated.ts")
Qt_add_translation(qm_files ${translation_files})
add_custom_target(compile_translations ALL DEPENDS ${qm_files})
