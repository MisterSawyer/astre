include("${PROJECT_SOURCE_DIR}/cmake/add_module.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/protobuf_generate.cmake")


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
        astre::Async
        astre::Process
)

protobuf_generate(NAME "Input_proto"
    PROTO_ROOT
        ${PROJECT_SOURCE_DIR}/engine/modules
    PROTO_SRC_DIRS   
        ${CMAKE_CURRENT_SOURCE_DIR}/proto
    PROTO_DST_DIR
        ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(Input PUBLIC "Input_proto")