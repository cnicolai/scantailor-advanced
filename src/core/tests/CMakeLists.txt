set(sources
    main.cpp
    TestContentSpanFinder.cpp
    TestDeskewParams.cpp
    TestObliqueFinder.cpp
    TestImageId.cpp
    TestImageSizeLimit.cpp
    TestMargins.cpp
    TestPageId.cpp
    TestPageRange.cpp
    TestPageSequence.cpp
    TestSelectContentApply.cpp
    TestSmartFilenameOrdering.cpp
    TestUnits.cpp)

add_executable(core_tests ${sources})
target_link_libraries(
    core_tests
    PRIVATE core test_support Boost::unit_test_framework)

add_test(NAME core_tests COMMAND core_tests --log_level=message)