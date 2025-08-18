include("${CMAKE_SOURCE_DIR}/cmake/add_module.cmake")

add_module(NAME "Render"
    INCLUDE_DIRS
        "include/render"
        "include/render/opengl"

    SOURCE_DIRS
        "src"
        "src/opengl"

    DEPENDENCIES
        asio
        glew
        #glm
        absl::flat_hash_map
        spdlog::spdlog

        astre::Math
        astre::Native
        astre::Async
        astre::Formatter
        astre::Type
        astre::Process
        astre::Window
)

protobuf_generate(NAME "Render_proto"
    PROTO_ROOT
        ${PROJECT_SOURCE_DIR}/modules
    PROTO_SRC_DIRS   
        ${CMAKE_CURRENT_SOURCE_DIR}/proto
    PROTO_DST_DIR
        ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries("Render_proto" PUBLIC "Math_proto")

target_link_libraries("Render" PUBLIC "Render_proto")
