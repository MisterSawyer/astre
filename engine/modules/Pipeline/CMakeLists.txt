include("${PROJECT_SOURCE_DIR}/cmake/add_module.cmake")

add_module(NAME "Pipeline"
    INCLUDE_DIRS
        "include"
    SOURCE_DIRS
        "src"
    
    DEPENDENCIES
        asio
        
        astre::Native
        astre::Process
        astre::Window
        astre::Render
        astre::ECS
        astre::Asset
)