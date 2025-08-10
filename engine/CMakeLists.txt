# --------------------------------------------------------------------------
# Engine CMakeLists
# --------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.30)

project("AstreEngine"   
    VERSION "0.0.1"
    DESCRIPTION "Astre Engine"
    HOMEPAGE_URL "https://github.com/MisterSawyer/astre"
    LANGUAGES C CXX
)

# --------------------------------------------------------------------------
# Options
# --------------------------------------------------------------------------
option(ASTRE_ENABLE_INSTALL "Enable install rule" OFF)
option(ASTRE_BUILD_TESTS "Build tests" OFF)
# --------------------------------------------------------------------------

set(PROJECT_PREFIX "astre")

add_subdirectory(modules)

# create static library target
add_library("${PROJECT_NAME}" STATIC
    "include/astre.hpp"
    "src/astre.cpp")

target_include_directories("${PROJECT_NAME}" 
    PUBLIC 
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>")

target_link_libraries("${PROJECT_NAME}"
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
        astre::GUI

    PUBLIC
        spdlog
        asio
        glew
        glm
)

target_sources("${PROJECT_NAME}" PUBLIC
    FILE_SET    public_headers
    TYPE        HEADERS
    BASE_DIRS   include
    FILES       "include/astre.hpp"
)

# static library install target
install(TARGETS "${PROJECT_NAME}"
    EXPORT "${PROJECT_NAME}Targets"
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILE_SET public_headers
)

# Generate version config
write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Generate config file
configure_package_config_file(
    "${CMAKE_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
    "${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

# Install all config files
install(
    FILES
        "${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        "${CMAKE_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

# build tests
if(ASTRE_BUILD_TESTS)
    add_subdirectory(tests)
endif()