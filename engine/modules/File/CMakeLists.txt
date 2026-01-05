include("${CMAKE_SOURCE_DIR}/cmake/add_module.cmake")

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

        astre::File_proto
        astre::Render_proto
        astre::ECS_proto
        astre::Script_proto
)