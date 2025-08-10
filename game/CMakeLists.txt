# --------------------------------------------------------------------------
# Game CMakeLists
# --------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.30)

project("AstreGame"   
    VERSION "0.0.1"
    DESCRIPTION "Astre Game"
    HOMEPAGE_URL "https://github.com/MisterSawyer/astre"
    LANGUAGES C CXX
)

set(PROJECT_PREFIX "astre_game")

# Name the target executable
add_executable("${PROJECT_NAME}"
    "src/main.cpp"
)

# Link against the Engine static library
target_link_libraries("${PROJECT_NAME}"
    PRIVATE
        AstreEngine
)

# Install game executable
install(TARGETS "${PROJECT_NAME}"
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# make install target of resources directory
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources/"
    DESTINATION "${CMAKE_INSTALL_BINDIR}/resources/"
)