cmake_minimum_required(VERSION 3.30)

set(PROJECT_NAME "Astre")
set(PROJECT_PREFIX "astre")

project("${PROJECT_NAME}"   
    VERSION "0.0.1"
    DESCRIPTION "Astre"
    HOMEPAGE_URL "https://github.com/MisterSawyer/astre"
    LANGUAGES C CXX
)

# --------------------------------------------------------------------------
# Options
# --------------------------------------------------------------------------
option(ASTRE_ENABLE_INSTALL "Enable install rule" OFF)
option(ASTRE_BUILD_TESTS "Build tests" OFF)

# --------------------------------------------------------------------------
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(cmake/compile_options.cmake)
include(cmake/dependencies.cmake)

message(STATUS "install directory ${CMAKE_INSTALL_PREFIX}")
message(STATUS "include/ directory ${CMAKE_INSTALL_INCLUDEDIR}")
message(STATUS "lib/ directory ${CMAKE_INSTALL_LIBDIR}")
message(STATUS "bin/ directory ${CMAKE_INSTALL_BINDIR}")

# Add subprojects
add_subdirectory(engine)
add_subdirectory(game)

# Generate version config
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Generate config file
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

# Install all config files
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

# Generate current timestamp
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)
file(WRITE "${CMAKE_BINARY_DIR}/build_timestamp" ${BUILD_TIMESTAMP})
install(FILES "${CMAKE_BINARY_DIR}/build_timestamp"
        DESTINATION ${CMAKE_INSTALL_BINDIR})