set(sources
    main.cpp
    TestBinaryImage.cpp TestReduceThreshold.cpp
    TestSlicedHistogram.cpp
    TestConnCompEraser.cpp TestConnCompEraserExt.cpp
    TestDpi.cpp
    TestGrayscale.cpp
    TestRasterOp.cpp TestShear.cpp
    TestOrthogonalRotation.cpp
    TestSkewFinder.cpp
    TestScale.cpp
    TestTransform.cpp
    TestMorphology.cpp
    TestBinarize.cpp
    TestPolygonRasterizer.cpp
    TestSeedFill.cpp
    TestSEDM.cpp
    TestRastLineFinder.cpp
    Utils.cpp Utils.h)

add_executable(imageproc_tests ${sources})
target_link_libraries(
    imageproc_tests
    PRIVATE imageproc math foundation test_support Qt6::Widgets Qt6::Xml
    Boost::unit_test_framework)

add_test(NAME imageproc_tests COMMAND imageproc_tests --log_level=message)
