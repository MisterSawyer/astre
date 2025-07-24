include("${PROJECT_SOURCE_DIR}/cmake/add_module.cmake")

set(INCLUDE_DIRS "include/process")
if(WIN32)
    message(STATUS "Configuring module Process for Windows")
    list(APPEND INCLUDE_DIRS "include/process/windows")
    set(SOURCE_DIRS "src/windows")
elseif(UNIX AND NOT APPLE)
    message(STATUS "Configuring module Process for Unix")
    list(APPEND INCLUDE_DIRS "include/process/unix")
    set(SOURCE_DIRS "src/unix")
elseif(APPLE)
    message(STATUS "Configuring module Process for macOS")
    set(INCLUDE_DIRS "include/process/mac")
    list(APPEND INCLUDE_DIRS "include/process/mac")
    set(SOURCE_DIRS "src/mac")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()


add_module(NAME "Process"
    INCLUDE_DIRS
        ${INCLUDE_DIRS}

    SOURCE_DIRS
        ${SOURCE_DIRS}

    DEPENDENCIES
        asio
        glew
        absl::flat_hash_map
        
        astre::Native
        astre::Async

        astre::Formatter
        astre::Type
)