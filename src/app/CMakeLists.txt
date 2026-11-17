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
    ProjectSaveFlow.cpp ProjectSaveFlow.h
    ThumbnailSequence.cpp ThumbnailSequence.h
    ProjectFilesDialog.cpp ProjectFilesDialog.h
    NewOpenProjectPanel.cpp NewOpenProjectPanel.h
    SystemLoadWidget.cpp SystemLoadWidget.h
    MainWindow.cpp MainWindow.h
    MainWindowActions.cpp
    MainWindowBatch.cpp
    MainWindowPages.cpp
    MainWindowProject.cpp
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

# The GUI sources as a library, so the app, ui_snapshot, qt_tests and the live Mac check share one compile (closes
# Phase 3 review finding WR-01). The autogen include directory is public because the app headers include generated
# ui_*.h files.
add_library(app_gui STATIC ${gui_only_sources} ${gui_only_ui_files})
target_link_libraries(app_gui PUBLIC core)
target_include_directories(
    app_gui
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/app_gui_autogen/include"
    "$<TARGET_PROPERTY:TIFF::TIFF,INTERFACE_INCLUDE_DIRECTORIES>")

add_executable(${target_name} main.cpp ${resource_files})
target_link_libraries(${target_name} PRIVATE app_gui)

# Names the application menu items after the app instead of the process (RESEARCH Pitfall 2). The plist is embedded as a
# section of the executable and stays in the source tree: an Info.plist next to the executable would fake a bundle.
set(info_plist "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")
target_link_options(${target_name} PRIVATE "LINKER:-sectcreate,__TEXT,__info_plist,${info_plist}")
set_property(TARGET ${target_name} APPEND PROPERTY LINK_DEPENDS "${info_plist}")

# Offscreen UI snapshot tool for Phase 3 D-12; built with the tests, not registered with ctest (TESTING.md lists
# seven suites). It links the shared app_gui library, so the GUI sources are compiled once.
if (BUILD_TESTS)
  add_executable(ui_snapshot ui_snapshot/main.cpp ${resource_files})
  target_link_libraries(ui_snapshot PRIVATE app_gui)
  target_compile_definitions(ui_snapshot PRIVATE UI_SNAPSHOT_ROOT="${CMAKE_SOURCE_DIR}/build")

  # Live Retina check of Phase 4 (D-02) on the Cocoa platform. It links core and test_support only, so the GUI sources
  # are not compiled a second time. Not registered with ctest (the
  # seven suites stay).
  add_executable(retina_check retina_check/main.cpp ${resource_files})
  target_link_libraries(retina_check PRIVATE core dewarping test_support)
  target_include_directories(
      retina_check
      PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
      "$<TARGET_PROPERTY:TIFF::TIFF,INTERFACE_INCLUDE_DIRECTORIES>")
  target_compile_definitions(retina_check PRIVATE RETINA_CHECK_ROOT="${CMAKE_SOURCE_DIR}/build")

  # Live Mac check of Phase 5 (D-12) on the Cocoa platform: application menu, key chords on the German layout and
  # the app's storage identity. Objective-C++ lives only here (CocoaProbe.mm), never in the app (D-00b). Not
  # registered with ctest.
  enable_language(OBJCXX)
  add_executable(mac_check mac_check/main.cpp mac_check/CocoaProbe.h mac_check/CocoaProbe.mm ${resource_files})
  target_link_libraries(mac_check PRIVATE app_gui dewarping "-framework AppKit" "-framework Carbon")
  target_compile_definitions(mac_check PRIVATE MAC_CHECK_ROOT="${CMAKE_SOURCE_DIR}/build")
  set_target_properties(mac_check PROPERTIES OBJCXX_STANDARD 17)
  set_source_files_properties(mac_check/CocoaProbe.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
  # The same embedded name as the app, so the harness shows what the app shows.
  target_link_options(mac_check PRIVATE "LINKER:-sectcreate,__TEXT,__info_plist,${info_plist}")
  set_property(TARGET mac_check APPEND PROPERTY LINK_DEPENDS "${info_plist}")
endif()
