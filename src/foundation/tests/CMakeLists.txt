set(sources
    main.cpp
    TestConstants.cpp
    TestLineIntersectionScalar.cpp
    TestProximity.cpp
    TestUtils.cpp
    TestXmlEscape.cpp)

add_executable(foundation_tests ${sources})
target_link_libraries(
    foundation_tests
    PRIVATE foundation Boost::unit_test_framework)

add_test(NAME foundation_tests COMMAND foundation_tests --log_level=message)
