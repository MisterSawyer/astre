include("${PROJECT_SOURCE_DIR}/cmake/add_module.cmake")

add_module(NAME "Async"
    INCLUDE_DIRS
        "include"
    SOURCE_DIRS
        "src"
    
    DEPENDENCIES
        asio

        astre::Native
)