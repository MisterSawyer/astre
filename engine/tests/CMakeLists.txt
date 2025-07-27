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
    "modules/Async/async_tests.cpp"

    "modules/Native/native_tests.cpp"
    "modules/Type/type_tests.cpp"
    "modules/Process/process_tests.cpp"

    "modules/Render/opengl_renderer_tests.cpp"
    "modules/Render/opengl_vertex_buffer_tests.cpp"
    "modules/Render/opengl_texture_tests.cpp"
    "modules/Render/opengl_shader_tests.cpp"

    "modules/World/save_archive_tests.cpp"
    
)

if(WIN32)
    target_sources(${PROJECT_NAME}Tests
        PRIVATE
            "modules/Process/winapi_process_tests.cpp"
            "modules/Window/winapi_window_tests.cpp"
        )
endif()

target_include_directories(
    ${PROJECT_NAME}Tests
    PRIVATE
        "include"
)

# Link
target_link_libraries(${PROJECT_NAME}Tests
    PRIVATE
        asio
        glew
        glm
        GTest::gtest_main
        GTest::gmock

        AstreEngine
)

# Discover tests
gtest_discover_tests(${PROJECT_NAME}Tests)


# === Install target ===
install(TARGETS ${PROJECT_NAME}Tests
    EXPORT ${PROJECT_NAME}TestsTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
