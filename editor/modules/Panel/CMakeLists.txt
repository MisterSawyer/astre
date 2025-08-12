include("${CMAKE_SOURCE_DIR}/cmake/add_module.cmake")

add_module(NAME "Panel"
    INCLUDE_DIRS
        "include"
    SOURCE_DIRS
        "src"
    
    DEPENDENCIES
        AstreEngine

        astre_editor::Controller
)