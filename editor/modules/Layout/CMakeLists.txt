include("${CMAKE_SOURCE_DIR}/cmake/add_module.cmake")

add_module(NAME "Layout"
    INCLUDE_DIRS
        "include"
    SOURCE_DIRS
        "src"
    
    DEPENDENCIES
        AstreEngine
)