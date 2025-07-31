include("${PROJECT_SOURCE_DIR}/cmake/add_module.cmake")

add_module(NAME "Script"
    INCLUDE_DIRS
        "include"
    SOURCE_DIRS
        "src"
    
    DEPENDENCIES
        asio
        glm
        spdlog
        absl::flat_hash_map
        lua_static
        sol2::sol2

        # astre::Async
        # astre::Native
        # astre::Process
        astre::Math
        ECS_proto

        # astre::Window
        # astre::Render
        # astre::ECS
        astre::Input
)