include("${CMAKE_SOURCE_DIR}/cmake/add_module.cmake")

add_module(NAME "Asset"
    INCLUDE_DIRS
        "include"
    SOURCE_DIRS
        "src"
    
    DEPENDENCIES
        asio
        absl::flat_hash_map
        spdlog
        
        astre::Native
        astre::Type
        astre::Render
        astre::ECS
        astre::Script
)