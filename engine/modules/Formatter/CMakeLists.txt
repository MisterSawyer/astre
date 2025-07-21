include("${PROJECT_SOURCE_DIR}/cmake/add_module.cmake")

add_module(NAME "Formatter"
    INCLUDE_DIRS
        "include/formatter"
    SOURCE_DIRS
        "src"
    
    DEPENDENCIES
        glew
    
        astre::Native
)