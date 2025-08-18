include("${CMAKE_SOURCE_DIR}/cmake/add_module.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/protobuf_generate.cmake")

add_module(NAME "File"
    INCLUDE_DIRS
        "include"
    SOURCE_DIRS
        "src"
    
    DEPENDENCIES
        asio
        absl::flat_hash_map
        spdlog

        astre::Async
        astre::Math
        astre::Native
        astre::Type
        astre::Process

        Render_proto
)

protobuf_generate(NAME "File_proto"
    PROTO_ROOT
        ${PROJECT_SOURCE_DIR}/modules
    PROTO_SRC_DIRS   
        ${CMAKE_CURRENT_SOURCE_DIR}/proto
    PROTO_DST_DIR
        ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(File_proto PUBLIC "ECS_proto")

target_link_libraries(File PUBLIC "File_proto")