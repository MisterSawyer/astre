# --------------------------------------------------------------------------
# Game CMakeLists
# --------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.30)

# Name the target executable
add_executable("${PROJECT_NAME}Game"
    "src/main.cpp"
)

# Link against the Engine static library
target_link_libraries("${PROJECT_NAME}Game"
    PRIVATE
        "${PROJECT_NAME}Engine"
)

# Install game executable
install(TARGETS "${PROJECT_NAME}Game"
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# make install target of resources directory
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources/"
    DESTINATION "${CMAKE_INSTALL_BINDIR}/resources/"
)