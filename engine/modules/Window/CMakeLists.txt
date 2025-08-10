include("${CMAKE_SOURCE_DIR}/cmake/add_module.cmake")

set(INCLUDE_DIRS "include/window")
if(WIN32)
    message(STATUS "Configuring module Window for Windows")
    list(APPEND INCLUDE_DIRS "include/window/windows")
    set(SOURCE_DIRS "src/windows")
elseif(UNIX AND NOT APPLE)
    message(STATUS "Configuring module Window for Unix")
    list(APPEND INCLUDE_DIRS "include/window/unix")
    set(SOURCE_DIRS "src/unix")
elseif(APPLE)
    message(STATUS "Configuring module Window for macOS")
    set(INCLUDE_DIRS "include/window/mac")
    list(APPEND INCLUDE_DIRS "include/window/mac")
    set(SOURCE_DIRS "src/mac")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

add_module(NAME "Window"
    INCLUDE_DIRS
        ${INCLUDE_DIRS}

    SOURCE_DIRS
        ${SOURCE_DIRS}

    DEPENDENCIES
        asio
        
        astre::Native
        astre::Async
        astre::Formatter
        astre::Type
        astre::Process
)