# --------------------------------------------------------------------------
# Engine Tests CMakeLists
# --------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.30)

# Enable testing
enable_testing()
include(GoogleTest)

# Add test executable
add_executable(${PROJECT_NAME}Tests
    
    "src/unit_tests.cpp"

    # Components Tests
    "modules/Native/native_tests.cpp"
    "modules/Type/type_tests.cpp"
    "modules/Process/process_tests.cpp"
)

if(WIN32)
    target_sources(${PROJECT_NAME}Tests
        PRIVATE
            "modules/Process/winapi_process_tests.cpp")
endif()

target_include_directories(
    ${PROJECT_NAME}Tests
    PRIVATE
        "include"
)

# Link
target_link_libraries(${PROJECT_NAME}Tests
    PRIVATE
        AstreEngine
        GTest::gtest_main
)

# Discover tests
gtest_discover_tests(${PROJECT_NAME}Tests)
