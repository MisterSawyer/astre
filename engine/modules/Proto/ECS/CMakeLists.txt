include("${CMAKE_SOURCE_DIR}/cmake/protobuf_generate.cmake")

protobuf_generate(NAME "ECS_proto"
    PROTO_ROOT
        ${PROJECT_SOURCE_DIR}/modules/Proto
    PROTO_SRC_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/components
    PROTO_DST_DIR
        ${CMAKE_BINARY_DIR}/generated/proto/
)