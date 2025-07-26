include("${PROJECT_SOURCE_DIR}/cmake/add_module.cmake")

add_module(NAME "World"
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
        astre::ECS
        astre::Asset
)