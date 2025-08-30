include("${CMAKE_SOURCE_DIR}/cmake/add_module.cmake")

add_module(NAME "Math"
    INCLUDE_DIRS
        "include"
    SOURCE_DIRS
        "src"
    
    DEPENDENCIES
        asio
        glm

        astre::Math_proto
)