include("${PROJECT_SOURCE_DIR}/cmake/add_module.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/protobuf_generate.cmake")

add_module(NAME "Math"
    INCLUDE_DIRS
        "include"
    SOURCE_DIRS
        "src"
    
    DEPENDENCIES
        asio
        glm
)

protobuf_generate(NAME "Math_proto"
    PROTO_ROOT
        ${PROJECT_SOURCE_DIR}/engine/modules
    PROTO_SRC_DIRS   
        ${CMAKE_CURRENT_SOURCE_DIR}/proto
    PROTO_DST_DIR
        ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(Math PUBLIC "Math_proto")