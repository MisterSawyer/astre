include("${CMAKE_SOURCE_DIR}/cmake/add_module.cmake")

add_module(NAME "Type"
    INCLUDE_DIRS
        "include/type"
    SOURCE_DIRS
        "src"
    
    DEPENDENCIES
        asio
        
        astre::Native
)