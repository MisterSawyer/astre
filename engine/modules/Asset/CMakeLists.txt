include("${CMAKE_SOURCE_DIR}/cmake/add_module.cmake")

add_module(NAME "Asset"
    INCLUDE_DIRS
        "include"
    SOURCE_DIRS
        "src"

    DEPENDENCIES
        asio
        absl::flat_hash_map
        absl::flat_hash_set
        absl::node_hash_map
        spdlog

        astre::Async
        astre::Native
        astre::Process
        astre::Math
        astre::Type
        astre::File
        astre::ECS

        astre::Render_proto
        astre::Script_proto
        astre::File_proto
        astre::ECS_proto
)
