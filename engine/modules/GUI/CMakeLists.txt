include("${PROJECT_SOURCE_DIR}/cmake/add_module.cmake")

add_module(NAME "GUI"
    INCLUDE_DIRS
        "include/gui"
    SOURCE_DIRS
        "src"
    
    DEPENDENCIES
        asio
        imgui

        astre::Native
        astre::Async
        astre::Render
        astre::Window

)