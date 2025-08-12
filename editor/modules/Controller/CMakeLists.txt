include("${CMAKE_SOURCE_DIR}/cmake/add_module.cmake")

add_module(NAME "Controller"
    INCLUDE_DIRS
        "include"
    SOURCE_DIRS
        "src"
    
    DEPENDENCIES
        AstreEngine
)