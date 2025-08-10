# --------------------------------------------------------------------------
# Editor CMakeLists
# --------------------------------------------------------------------------

set(PROJECT_PREFIX "astre_editor")

project("AstreEditor"   
    VERSION "0.0.1"
    DESCRIPTION "Astre Editor"
    HOMEPAGE_URL "https://github.com/MisterSawyer/astre"
    LANGUAGES C CXX
)

# --------------------------------------------------------------------------
# Options
# --------------------------------------------------------------------------
option(ASTRE_ENABLE_INSTALL "Enable install rule" OFF)
option(ASTRE_EDITOR_BUILD_TESTS "Build tests" OFF)
# --------------------------------------------------------------------------

set(PROJECT_PREFIX "astre_editor")

include("${CMAKE_SOURCE_DIR}/cmake/compile_options.cmake")

add_subdirectory(modules)

# Name the target executable
add_executable("${PROJECT_NAME}"
    "include/astre_editor.hpp"
    "src/main.cpp"
)

target_include_directories("${PROJECT_NAME}" 
    PUBLIC 
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>")

# Link against the Engine static library
target_link_libraries("${PROJECT_NAME}"
    PRIVATE
        AstreEngine
    PRIVATE
        astre_editor::Layout
        astre_editor::Panel
)

# Install Editor executable
target_sources("${PROJECT_NAME}" PUBLIC
    FILE_SET    public_headers
    TYPE        HEADERS
    BASE_DIRS   include
    FILES       "include/astre_editor.hpp"
)

# static library install target
install(TARGETS "${PROJECT_NAME}"
    EXPORT "${PROJECT_NAME}Targets"
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILE_SET public_headers
)


# Generate version config
write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# make install target of resources directory
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources/"
    DESTINATION "${CMAKE_INSTALL_BINDIR}/resources/"
)

# build tests
if(ASTRE_EDITOR_BUILD_TESTS)
    add_subdirectory(tests)
endif()