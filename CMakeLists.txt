cmake_minimum_required(VERSION 3.30)

project("Astre"   
    VERSION "0.0.1"
    DESCRIPTION "Astre"
    HOMEPAGE_URL "https://github.com/MisterSawyer/astre"
    LANGUAGES C CXX
)

message(STATUS "install directory ${CMAKE_INSTALL_PREFIX}")
message(STATUS "include/ directory ${CMAKE_INSTALL_INCLUDEDIR}")
message(STATUS "lib/ directory ${CMAKE_INSTALL_LIBDIR}")
message(STATUS "bin/ directory ${CMAKE_INSTALL_BINDIR}")

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include("${CMAKE_SOURCE_DIR}/cmake/compile_options.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/dependencies.cmake")

# Add subprojects
add_subdirectory(engine)
add_subdirectory(editor)
add_subdirectory(game)

# Generate current timestamp
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)
file(WRITE "${CMAKE_BINARY_DIR}/build_timestamp" ${BUILD_TIMESTAMP})
install(FILES "${CMAKE_BINARY_DIR}/build_timestamp"
        DESTINATION ${CMAKE_INSTALL_BINDIR})