include("${PROJECT_SOURCE_DIR}/cmake/add_module.cmake")

set(INCLUDE_DIRS "include/native")
if(WIN32)
    message(STATUS "Configuring module Native for Windows")
    list(APPEND INCLUDE_DIRS "include/native/windows")
    set(SOURCE_DIRS "src/windows")
    set(DEPENDENCIES ws2_32)
elseif(UNIX AND NOT APPLE)
    message(STATUS "Configuring module Native for Unix")
    list(APPEND INCLUDE_DIRS "include/native/unix")
    set(SOURCE_DIRS "src/unix")
elseif(APPLE)
    message(STATUS "Configuring module Native for macOS")
    set(INCLUDE_DIRS "include/native/mac")
    list(APPEND INCLUDE_DIRS "include/native/mac")
    set(SOURCE_DIRS "src/mac")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()


add_module(NAME "Native"
    INCLUDE_DIRS
        ${INCLUDE_DIRS}

    SOURCE_DIRS
        ${SOURCE_DIRS}

    DEPENDENCIES
        ${DEPENDENCIES}
        asio
        spdlog::spdlog
)

