include("${PROJECT_SOURCE_DIR}/cmake/add_module.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/protobuf_generate.cmake")

add_module(NAME "ECS"
    INCLUDE_DIRS
        "include"
    SOURCE_DIRS
        "src"
    
    DEPENDENCIES
        asio
        absl::flat_hash_map

        astre::Math
        astre::Native
        astre::Type
        astre::Input
)

protobuf_generate(NAME "ECS_proto"
    PROTO_ROOT
        ${PROJECT_SOURCE_DIR}/engine/modules
    PROTO_SRC_DIRS   
        ${CMAKE_CURRENT_SOURCE_DIR}/proto
        ${CMAKE_CURRENT_SOURCE_DIR}/proto/components
    PROTO_DST_DIR
        ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(ECS_proto PUBLIC Math_proto Input_proto)

target_link_libraries(ECS PUBLIC "ECS_proto")