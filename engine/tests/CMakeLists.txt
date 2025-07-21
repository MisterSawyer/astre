# --------------------------------------------------------------------------
# Engine Tests CMakeLists
# --------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.30)

# Enable testing
enable_testing()
include(GoogleTest)

# Add test executable
add_executable(${PROJECT_NAME}Tests
    "modules/Native/native_tests.cpp"
)

# Link
target_link_libraries(${PROJECT_NAME}Tests
    PRIVATE
        AstreEngine
        GTest::gtest_main
)

# Discover tests
gtest_discover_tests(${PROJECT_NAME}Tests)
