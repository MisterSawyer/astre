cmake_minimum_required(VERSION 3.30)

include(FetchContent)
include(ExternalProject)
include(cmake/silence_warnings.cmake)

# ---------------------------------------------------------
# spdlog
# ---------------------------------------------------------
message(STATUS "Fetching dependency `spdlog` ...")
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY  "https://github.com/gabime/spdlog.git"
    GIT_TAG         "v1.11.0"
    SYSTEM
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(spdlog)
# Suppress warnings for building spdlog
silence_warnings(TARGETS spdlog spdlog::spdlog)

# ---------------------------------------------------------
# asio
# ---------------------------------------------------------
message(STATUS "Fetching dependency `asio` ...")
FetchContent_Declare(
    asio
    GIT_REPOSITORY "https://github.com/chriskohlhoff/asio.git"
    GIT_TAG        "asio-1-34-0"
    SYSTEM
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(asio)
add_library(asio INTERFACE)
if(WIN32)
    target_compile_options(asio INTERFACE /wd4459)
endif()
target_include_directories(asio INTERFACE "${asio_SOURCE_DIR}/asio/include")
install(DIRECTORY ${asio_SOURCE_DIR}/asio/include/asio DESTINATION include)

# ---------------------------------------------------------
# abseil
# ---------------------------------------------------------
message(STATUS "Fetching dependency `abseil` ...")
FetchContent_Declare(
    abseil
    GIT_REPOSITORY "https://github.com/abseil/abseil-cpp.git"
    GIT_TAG "20250127.1"
    SYSTEM
    OVERRIDE_FIND_PACKAGE
)
set(ABSL_MSVC_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
set(ABSL_ENABLE_INSTALL ON CACHE BOOL "" FORCE)
set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(abseil)

# Suppress warnings for building asio
silence_warnings(TARGETS 
    absl::algorithm
    absl::base
    absl::debugging
    absl::flat_hash_map
    absl::flags
    absl::memory
    absl::meta
    absl::numeric
    absl::random_random
    absl::strings
    absl::synchronization
    absl::time
    absl::utility
    absl::malloc_internal
    absl::time_zone
)

# ---------------------------------------------------------
# protobuf
# ---------------------------------------------------------
message(STATUS "Fetching dependency `protobuf` ...")
FetchContent_Declare(
        protobuf
        GIT_REPOSITORY "https://github.com/protocolbuffers/protobuf"
        GIT_TAG        "v30.2"
        SYSTEM
        OVERRIDE_FIND_PACKAGE
)
set(protobuf_MSVC_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(protobuf_ABSL_PROVIDER "package" CACHE STRING "" FORCE)
set(absl_DIR "${abseil_SOURCE_DIR}" CACHE PATH "")
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_CONFORMANCE OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(protobuf)

# Suppress warnings for building protobuf
silence_warnings(TARGETS 
    protobuf::libprotobuf
    protobuf::libprotoc
    protobuf::protoc
    protobuf::protoc-gen-upb
    protobuf::protoc-gen-upb_minitable
    protobuf::protoc-gen-upbdefs
    protobuf::libupb
    protobuf::libprotobuf-lite
    utf8_range
    utf8_validity
)
# set protobuf protoc executable location
set(Protobuf_PROTOC_EXECUTABLE $<TARGET_FILE:protobuf::protoc> CACHE FILEPATH "" FORCE)
    
add_custom_target(check_protoc_version
    COMMAND "${Protobuf_PROTOC_EXECUTABLE}" --version
    COMMENT "Checking protoc version ${Protobuf_PROTOC_EXECUTABLE}"
    VERBATIM
)

# ---------------------------------------------------------
# json
# ---------------------------------------------------------
message(STATUS "Fetching dependency `json` ...")
FetchContent_Declare(
    json
    GIT_REPOSITORY  "https://github.com/nlohmann/json.git"
    GIT_TAG         "v3.12.0"
    SYSTEM
    OVERRIDE_FIND_PACKAGE
)
set(JSON_BuildTests OFF)
FetchContent_MakeAvailable(json)
silence_warnings(TARGETS nlohmann_json)

# ---------------------------------------------------------
# GLEW
# ---------------------------------------------------------
message(STATUS "Fetching dependency `GLEW` ...")

set(GLEW_PREFIX         ${CMAKE_BINARY_DIR}/_deps/glew)
set(GLEW_SOURCE_DIR     ${GLEW_PREFIX}/src)
set(GLEW_BUILD_DIR      ${GLEW_PREFIX}/build)
set(GLEW_INSTALL_DIR    ${GLEW_PREFIX}/install)

# Compiler-agnostic CMake options
set(GLEW_CMAKE_ARGS
    -S ${GLEW_SOURCE_DIR}/build/cmake
    -B ${GLEW_BUILD_DIR}
    -DCMAKE_INSTALL_PREFIX=${GLEW_INSTALL_DIR}
    -DBUILD_UTILS=OFF
    -DGLEW_STATIC=ON
    -DBUILD_SHARED_LIBS=OFF
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5
)

# Compiler-specific overrides
if(MSVC)
    message(STATUS "Applying MSVC-specific flags for GLEW build")

    list(APPEND GLEW_CMAKE_ARGS
        -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
        -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>"
        -DCMAKE_C_FLAGS_RELEASE="/MT"
        -DCMAKE_C_FLAGS_DEBUG="/MTd"
        -DCMAKE_CXX_FLAGS_RELEASE="/MT"
        -DCMAKE_CXX_FLAGS_DEBUG="/MTd"
        -DCMAKE_STATIC_LINKER_FLAGS="/IGNORE:4281"
        -DCMAKE_SHARED_LINKER_FLAGS="/IGNORE:4281"
        -DCMAKE_MODULE_LINKER_FLAGS="/IGNORE:4281"
        -DCMAKE_EXE_LINKER_FLAGS="/IGNORE:4281"
    )

    set(GLEW_BUILD_COMMAND 
        ${CMAKE_COMMAND} --build ${GLEW_BUILD_DIR} --config Debug &&
        ${CMAKE_COMMAND} --build ${GLEW_BUILD_DIR} --config Release
    )

    set(GLEW_INSTALL_COMMAND
        ${CMAKE_COMMAND} --install ${GLEW_BUILD_DIR} --config Debug &&
        ${CMAKE_COMMAND} --install ${GLEW_BUILD_DIR} --config Release
    )

endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Applying GCC-specific flags for GLEW build")

    string(JOIN " " C_FLAGS_DEBUG "-O0" "-DDEBUG")
    string(JOIN " " C_FLAGS_RELEASE "-O3" "-DNDEBUG")
    string(JOIN " " CXX_FLAGS_DEBUG "-O0" "-DDEBUG")
    string(JOIN " " CXX_FLAGS_RELEASE "-O3" "-DNDEBUG")
    
    list(APPEND GLEW_CMAKE_ARGS
        "-DCMAKE_C_FLAGS_DEBUG=${C_FLAGS_DEBUG}"
        "-DCMAKE_C_FLAGS_RELEASE=${C_FLAGS_RELEASE}"
        "-DCMAKE_CXX_FLAGS_DEBUG=${CXX_FLAGS_DEBUG}"
        "-DCMAKE_CXX_FLAGS_RELEASE=${CXX_FLAGS_RELEASE}"
        "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    )
endif()

ExternalProject_Add(glew_build
    URL https://github.com/nigels-com/glew/releases/download/glew-2.2.0/glew-2.2.0.tgz
    URL_HASH SHA256=d4fc82893cfb00109578d0a1a2337fb8ca335b3ceccf97b97e5cc7f08e4353e1

    PREFIX        ${GLEW_PREFIX}
    SOURCE_DIR    ${GLEW_SOURCE_DIR}
    BINARY_DIR    ${GLEW_BUILD_DIR}
    INSTALL_DIR   ${GLEW_INSTALL_DIR}
    STAMP_DIR     ${GLEW_PREFIX}/stamp
    LOG_DIR       ${GLEW_PREFIX}/log
    DOWNLOAD_DIR  ${GLEW_PREFIX}/download

    CONFIGURE_COMMAND ${CMAKE_COMMAND} ${GLEW_CMAKE_ARGS}
    BUILD_COMMAND     ${GLEW_BUILD_COMMAND}
    INSTALL_COMMAND   ${GLEW_INSTALL_COMMAND}
    BUILD_ALWAYS      TRUE
    UPDATE_DISCONNECTED TRUE
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
)

# Interface library
add_library(glew INTERFACE)
target_include_directories(glew INTERFACE "${GLEW_INSTALL_DIR}/include")
target_link_directories(glew INTERFACE "${GLEW_INSTALL_DIR}/lib")

target_compile_definitions(glew INTERFACE GLEW_STATIC)

if(MSVC)
    target_link_libraries(glew INTERFACE
        $<$<CONFIG:Debug>:libglew32d>
        $<$<CONFIG:Release>:libglew32>
        $<$<CONFIG:RelWithDebInfo>:libglew32>
        $<$<CONFIG:MinSizeRel>:libglew32>
        glu32 opengl32)
else()
    target_link_libraries(glew INTERFACE libGLEW.a GL GLU)
endif()

add_dependencies(glew glew_build)

# Optional installation for consumers
install(DIRECTORY ${GLEW_INSTALL_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY ${GLEW_INSTALL_DIR}/lib/     DESTINATION ${CMAKE_INSTALL_LIBDIR})

# ---------------------------------------------------------
# glm
# ---------------------------------------------------------
message(STATUS "Fetching dependency `glm` ...")
FetchContent_Declare(
    glm
    GIT_REPOSITORY "https://github.com/g-truc/glm.git"
    GIT_TAG        "1.0.1"
    SYSTEM
)
FetchContent_MakeAvailable(glm)


# ---------------------------------------------------------
# lua
# ---------------------------------------------------------
message(STATUS "Fetching dependency `lua` ...")
FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/walterschell/Lua.git
    GIT_TAG v5.4.5
)
set(LUA_SUPPORT_DL OFF CACHE BOOL "" FORCE)
set(LUA_BUILD_AS_CXX OFF CACHE BOOL "" FORCE) # expose full C LUA API
set(LUA_ENABLE_SHARED OFF CACHE BOOL "" FORCE)
set(LUA_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(LUA_BUILD_BINARY OFF CACHE BOOL "" FORCE)
set(LUA_BUILD_COMPILER ON CACHE BOOL "" FORCE)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
FetchContent_MakeAvailable(lua)
silence_warnings(TARGETS lua_static luac)

# ---------------------------------------------------------
# lua API sol2 v3.5.0
# ---------------------------------------------------------
message(STATUS "Fetching dependency `lua api sol2` ...")
FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(sol2)

# ---------------------------------------------------------
# GTest
# ---------------------------------------------------------
if(ASTRE_BUILD_TESTS)
    set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
    message(STATUS "Fetching dependency `GTest` ...")
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY "https://github.com/google/googletest"
        GIT_TAG        "v1.16.0"
        SYSTEM
    )
    FetchContent_MakeAvailable(googletest)
    silence_warnings(TARGETS gtest gtest_main gmock gmock_main)
endif()
