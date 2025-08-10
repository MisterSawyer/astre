include("${CMAKE_SOURCE_DIR}/cmake/add_module.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/protobuf_generate.cmake")

add_module(NAME "ECS"
    INCLUDE_DIRS
        "include"
    SOURCE_DIRS
        "src"
        "src/system"
    
    DEPENDENCIES
        asio
        spdlog
        absl::flat_hash_map

        astre::Async
        astre::Math
        astre::Native
        astre::Type
        astre::Process
        astre::Render
        astre::Input
        astre::Script
)

protobuf_generate(NAME "ECS_proto"
    PROTO_ROOT
        ${PROJECT_SOURCE_DIR}/modules
    PROTO_SRC_DIRS   
        ${CMAKE_CURRENT_SOURCE_DIR}/proto
        ${CMAKE_CURRENT_SOURCE_DIR}/proto/components
    PROTO_DST_DIR
        ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(ECS_proto PUBLIC Math_proto Input_proto)

target_link_libraries(ECS PUBLIC "ECS_proto")