include("${CMAKE_SOURCE_DIR}/cmake/add_module.cmake")

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

        astre::ECS_proto
)