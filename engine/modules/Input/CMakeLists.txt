include("${CMAKE_SOURCE_DIR}/cmake/add_module.cmake")

set(INCLUDE_DIRS "include/input")
if(WIN32)
    message(STATUS "Configuring module Input for Windows")
    list(APPEND INCLUDE_DIRS "include/input/windows")
    set(SOURCE_DIRS "src/windows")
elseif(UNIX AND NOT APPLE)
    message(STATUS "Configuring module Input for Unix")
    list(APPEND INCLUDE_DIRS "include/input/unix")
    set(SOURCE_DIRS "src/unix")
elseif(APPLE)
    message(STATUS "Configuring module Input for macOS")
    list(APPEND INCLUDE_DIRS "include/input/mac")
    set(SOURCE_DIRS "src/mac")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

add_module(NAME "Input"
    INCLUDE_DIRS
        ${INCLUDE_DIRS}
    SOURCE_DIRS
        "src"
        ${SOURCE_DIRS}
    
    DEPENDENCIES
        asio
        spdlog
        absl::flat_hash_map

        astre::Native
        astre::Math
        astre::Async
        astre::Process

        astre::Input_proto
)