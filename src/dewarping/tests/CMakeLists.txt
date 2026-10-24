set(sources main.cpp TestDistortionModel.cpp TestCylindricalDewarper.cpp TestTextLineTracer.cpp)

add_executable(dewarping_tests ${sources})
target_link_libraries(
    dewarping_tests
    PRIVATE dewarping imageproc math foundation test_support Qt6::Xml Qt6::Gui Qt6::Core
    Boost::unit_test_framework)

add_test(NAME dewarping_tests COMMAND dewarping_tests --log_level=message)
set_tests_properties(dewarping_tests PROPERTIES ENVIRONMENT "LC_ALL=en_US.UTF-8")
