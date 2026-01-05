function(protobuf_generate)
    set(options)
    set(oneValueArgs NAME PROTO_DST_DIR PROTO_ROOT)
    set(multiValueArgs PROTO_SRC_DIRS)
    cmake_parse_arguments(protobuf_generate "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT Protobuf_PROTOC_EXECUTABLE)
        message(FATAL_ERROR "Protobuf not found. Please install protobuf or set Protobuf_PROTOC_EXECUTABLE.")
    endif()

    if(NOT protobuf_generate_PROTO_ROOT)
        message(FATAL_ERROR "PROTO_ROOT must be set for import path resolution.")
    endif()

    message(STATUS "Generating proto files of target `${protobuf_generate_NAME}`\n\tfrom:\t${protobuf_generate_PROTO_SRC_DIRS}\n\tto:\t${protobuf_generate_PROTO_DST_DIR}\n\troot:\t${protobuf_generate_PROTO_ROOT}")

    file(MAKE_DIRECTORY ${protobuf_generate_PROTO_DST_DIR})

    set(PROTO_GEN_SRCS "")
    set(PROTO_GEN_HDRS "")
    set(PROTO_FILES "")

    foreach(SRC_DIR IN LISTS protobuf_generate_PROTO_SRC_DIRS)
        file(GLOB FOUND_PROTOS "${SRC_DIR}/*.proto")
        list(APPEND PROTO_FILES ${FOUND_PROTOS})
    endforeach()

    if(PROTO_FILES STREQUAL "")
        message(WARNING "No .proto files found in: ${protobuf_generate_PROTO_SRC_DIRS}")
    endif()

    foreach(PROTO_FILE IN LISTS PROTO_FILES)
        file(RELATIVE_PATH REL_PATH "${protobuf_generate_PROTO_ROOT}" "${PROTO_FILE}")
        message( STATUS "relative path : ${REL_PATH}")
        if(REL_PATH MATCHES "^\.\./")
            message(FATAL_ERROR "Proto file ${PROTO_FILE} is not under PROTO_ROOT: ${protobuf_generate_PROTO_ROOT}")
        endif()

        get_filename_component(PROTO_NAME "${REL_PATH}" NAME_WE)
        get_filename_component(DIR_PART "${REL_PATH}" DIRECTORY)

        if(NOT "${DIR_PART}" STREQUAL "")
            set(GEN_SRC "${protobuf_generate_PROTO_DST_DIR}/${DIR_PART}/${PROTO_NAME}.pb.cc")
            set(GEN_HDR "${protobuf_generate_PROTO_DST_DIR}/${DIR_PART}/${PROTO_NAME}.pb.h")
        else()
            set(GEN_SRC "${protobuf_generate_PROTO_DST_DIR}/${PROTO_NAME}.pb.cc")
            set(GEN_HDR "${protobuf_generate_PROTO_DST_DIR}/${PROTO_NAME}.pb.h")
        endif()

        list(APPEND PROTO_GEN_SRCS ${GEN_SRC})
        list(APPEND PROTO_GEN_HDRS ${GEN_HDR})

        add_custom_command(
            OUTPUT ${GEN_SRC} ${GEN_HDR}
            COMMAND ${Protobuf_PROTOC_EXECUTABLE}
                    --proto_path=${protobuf_generate_PROTO_ROOT}
                    --cpp_out=${protobuf_generate_PROTO_DST_DIR}
                    "${PROTO_FILE}"
            DEPENDS "${PROTO_FILE}"
            WORKING_DIRECTORY "${protobuf_generate_PROTO_ROOT}"
            COMMENT "Generating C++ files from ${PROTO_FILE}"
            VERBATIM
        )

        set_source_files_properties(${GEN_SRC} PROPERTIES COMPILE_FLAGS "/wd4100")
    endforeach()

    message(STATUS "Generated proto files: ${PROTO_GEN_SRCS} ${PROTO_GEN_HDRS}")

    add_library(${protobuf_generate_NAME} OBJECT ${PROTO_GEN_SRCS} ${PROTO_GEN_HDRS})
    add_library("${PROJECT_PREFIX}::${protobuf_generate_NAME}" ALIAS "${protobuf_generate_NAME}")

    target_include_directories(${protobuf_generate_NAME} 
        PUBLIC
            "$<BUILD_INTERFACE:${protobuf_generate_PROTO_DST_DIR}>"

            "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated>"
            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/generated>"
    )

    target_link_libraries(${protobuf_generate_NAME} PUBLIC protobuf::libprotobuf)

    add_dependencies(${protobuf_generate_NAME} protobuf::protoc)

    install(DIRECTORY ${protobuf_generate_PROTO_DST_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/generated
        FILES_MATCHING PATTERN "*.h*"
    )
endfunction()
