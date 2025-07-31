# --------------------------------------------------------------------------
# Engine CMakeLists
# --------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.30)

add_subdirectory(src)
add_subdirectory(modules)

# create static library target
add_library("${PROJECT_NAME}Engine" STATIC
    "include/astre.hpp"
    "src/astre.cpp")

target_include_directories("${PROJECT_NAME}Engine" 
    PUBLIC 
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>")

target_link_libraries("${PROJECT_NAME}Engine"
    PUBLIC
        astre::Math
        Math_proto
        astre::Native
        astre::Async
        astre::Version
        astre::Entry
        astre::Type
        astre::Formatter
        astre::Process
        astre::Window
        astre::Render
        astre::Input
        Input_proto
        astre::ECS
        ECS_proto
        astre::Asset
        astre::Pipeline
        astre::World
        World_proto
        astre::Script
    PUBLIC
        spdlog
        asio
        glew
        glm   
)

target_sources("${PROJECT_NAME}Engine" PUBLIC
    FILE_SET    public_headers
    TYPE        HEADERS
    BASE_DIRS   include
    FILES       "include/astre.hpp"
)

# static library install target
install(TARGETS "${PROJECT_NAME}Engine"
    EXPORT "${PROJECT_NAME}EngineTargets"
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILE_SET public_headers
)

# build tests
if(ASTRE_BUILD_TESTS)
    add_subdirectory(tests)
endif()