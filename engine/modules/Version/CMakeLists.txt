include("${PROJECT_SOURCE_DIR}/cmake/add_module.cmake")


configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
    "${CMAKE_BINARY_DIR}/generated/include/version/version.h"
)

add_module(NAME "Version"
    INCLUDE_DIRS
        "include"
        "${CMAKE_BINARY_DIR}/generated/include"
    SOURCE_DIRS
        "src"
)

# Install the generated file
install(
    FILES "${CMAKE_BINARY_DIR}/generated/include/version/version.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/version"
)
