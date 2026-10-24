set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(target_name scantailor-advanced)

set(resource_files
    resources.qrc
    dark_scheme/dark_scheme.qrc
    light_scheme/light_scheme.qrc)
list_items_prepend(resource_files "${SCANTAILOR_RESOURCES_DIR}/")

set(gui_only_sources
    RelinkablePathVisualization.cpp RelinkablePathVisualization.h
    RelinkingModel.cpp RelinkingModel.h
    RelinkingSortingModel.cpp RelinkingSortingModel.h
    RelinkingListView.cpp RelinkingListView.h
    RelinkingDialog.cpp RelinkingDialog.h
    SettingsDialog.cpp SettingsDialog.h
    FixDpiDialog.cpp FixDpiDialog.h
    LoadFilesStatusDialog.cpp LoadFilesStatusDialog.h
    ProjectCreationContext.cpp ProjectCreationContext.h
    ProjectOpeningContext.cpp ProjectOpeningContext.h
    OutOfMemoryDialog.cpp OutOfMemoryDialog.h
    ThumbnailSequence.cpp ThumbnailSequence.h
    ProjectFilesDialog.cpp ProjectFilesDialog.h
    NewOpenProjectPanel.cpp NewOpenProjectPanel.h
    SystemLoadWidget.cpp SystemLoadWidget.h
    MainWindow.cpp MainWindow.h
    main.cpp
    StatusBarPanel.cpp StatusBarPanel.h
    DefaultParamsDialog.cpp DefaultParamsDialog.h)

set(gui_only_ui_files
    AboutDialog.ui
    BatchProcessingLowerPanel.ui
    DefaultParamsDialog.ui
    FixDpiDialog.ui
    LoadFilesStatusDialog.ui
    MainWindow.ui
    NewOpenProjectPanel.ui
    OutOfMemoryDialog.ui
    ProjectFilesDialog.ui
    RelinkingDialog.ui
    RemovePagesDialog.ui
    SettingsDialog.ui
    StatusBarPanel.ui
    SystemLoadWidget.ui)

add_executable(
    ${target_name} ${gui_only_sources} ${gui_only_ui_files}
    ${resource_files})
target_link_libraries(
    ${target_name}
    PRIVATE core)
target_include_directories(
    ${target_name}
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    "$<TARGET_PROPERTY:TIFF::TIFF,INTERFACE_INCLUDE_DIRECTORIES>")

translation_sources(${target_name} ${gui_only_sources} ${gui_only_ui_files})

# Offscreen UI snapshot tool for Phase 3 D-12; built with the tests, not registered with ctest (TESTING.md lists
# seven suites).
if (BUILD_TESTS)
  set(ui_snapshot_sources ${gui_only_sources})
  list(REMOVE_ITEM ui_snapshot_sources main.cpp)
  add_executable(
      ui_snapshot ui_snapshot/main.cpp ${ui_snapshot_sources}
      ${gui_only_ui_files} ${resource_files})
  target_link_libraries(ui_snapshot PRIVATE core)
  target_include_directories(
      ui_snapshot
      PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
      "$<TARGET_PROPERTY:TIFF::TIFF,INTERFACE_INCLUDE_DIRECTORIES>")
  target_compile_definitions(ui_snapshot PRIVATE UI_SNAPSHOT_ROOT="${CMAKE_SOURCE_DIR}/build")
endif()
