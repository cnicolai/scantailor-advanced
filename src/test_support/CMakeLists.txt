file(GLOB test_support_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(test_support STATIC ${test_support_sources})
target_link_libraries(test_support PUBLIC ${Qt_Gui_lib} ${Qt_Core_lib})
target_include_directories(test_support PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
