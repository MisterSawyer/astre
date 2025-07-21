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
silence_warnings(TARGETS spdlog::spdlog)

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
ExternalProject_Add(glew_build
    URL https://github.com/nigels-com/glew/releases/download/glew-2.2.0/glew-2.2.0.tgz
    URL_HASH SHA256=d4fc82893cfb00109578d0a1a2337fb8ca335b3ceccf97b97e5cc7f08e4353e1
    PREFIX ${CMAKE_BINARY_DIR}/_deps/glew
    SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/glew/src
    STAMP_DIR ${CMAKE_BINARY_DIR}/_deps/glew/stamp
    LOG_DIR ${CMAKE_BINARY_DIR}/_deps/glew/log
    DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/_deps/glew/download
    CONFIGURE_COMMAND 
        ${CMAKE_COMMAND} 
            -S ${CMAKE_BINARY_DIR}/_deps/glew/src/build/cmake
            -B ${CMAKE_BINARY_DIR}/_deps/glew/build
            -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>"
            -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
            -DCMAKE_C_FLAGS_RELEASE="/MT"
            -DCMAKE_C_FLAGS_DEBUG="/MTd"
            -DCMAKE_CXX_FLAGS_RELEASE="/MT"
            -DCMAKE_CXX_FLAGS_DEBUG="/MTd"
            -DCMAKE_C_FLAGS_MINSIZEREL="/MT"
            -DCMAKE_C_FLAGS_RELWITHDEBINFO="/MT"
            -DCMAKE_STATIC_LINKER_FLAGS="/IGNORE:4281"
            -DCMAKE_SHARED_LINKER_FLAGS="/IGNORE:4281"
            -DCMAKE_MODULE_LINKER_FLAGS="/IGNORE:4281"
            -DCMAKE_EXE_LINKER_FLAGS="/IGNORE:4281"
            -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/_deps/glew/install 
            -DBUILD_UTILS=OFF 
            -DGLEW_STATIC=ON 
            -DBUILD_SHARED_LIBS=OFF
            -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    
    BUILD_COMMAND 
        ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/_deps/glew/build --config Release
    BUILD_ALWAYS TRUE
    BINARY_DIR ${CMAKE_BINARY_DIR}/_deps/glew/build
    
    INSTALL_COMMAND 
        ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR}/_deps/glew/build
    
    UPDATE_DISCONNECTED TRUE
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
)
add_library(glew INTERFACE)
target_compile_options(glew INTERFACE /wd4459)
target_include_directories(glew INTERFACE "${CMAKE_BINARY_DIR}/_deps/glew/install/include")
target_link_directories(glew INTERFACE "${CMAKE_BINARY_DIR}/_deps/glew/install/lib")
target_link_libraries(glew INTERFACE libglew32 glu32 opengl32)
add_dependencies(glew glew_build)

install(DIRECTORY ${CMAKE_BINARY_DIR}/_deps/glew/install/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY ${CMAKE_BINARY_DIR}/_deps/glew/install/lib/     DESTINATION ${CMAKE_INSTALL_LIBDIR})

# ---------------------------------------------------------
# GTest
# ---------------------------------------------------------
if(ASTRE_BUILD_TESTS)
    message(STATUS "Fetching dependency `GTest` ...")
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY "https://github.com/google/googletest"
        GIT_TAG        "v1.16.0"
        SYSTEM
    )
    FetchContent_MakeAvailable(googletest)
endif()
