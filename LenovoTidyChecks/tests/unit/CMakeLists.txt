add_executable(lenovo_tidy_unit_tests
    StringUtilsTest.cpp
    NamingConventionHelperTest.cpp
)

target_link_libraries(lenovo_tidy_unit_tests
    PRIVATE
        lenovo_tidy_core
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(lenovo_tidy_unit_tests
    PROPERTIES
        LABELS "unit"
)
