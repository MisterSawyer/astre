include("${PROJECT_SOURCE_DIR}/cmake/add_module.cmake")

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