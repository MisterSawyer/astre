include("${PROJECT_SOURCE_DIR}/cmake/add_module.cmake")

add_module(NAME "Entry"
    INCLUDE_DIRS
        "include"
    SOURCE_DIRS
        "src"
    
    DEPENDENCIES
        asio
        spdlog
        
        astre::Version
        astre::Native
        astre::Process
)