function(add_module)
    set(options)
    set(oneValueArgs NAME)
    set(multiValueArgs INCLUDE_DIRS SOURCE_DIRS DEPENDENCIES)
    cmake_parse_arguments(add_module "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT add_module_NAME)
        message(FATAL_ERROR "add_module requires a NAME argument")
    endif()

    # === Resolve absolute include directories ===
    set(ABS_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include")
    foreach(INC_DIR IN LISTS add_module_INCLUDE_DIRS)
        if(IS_ABSOLUTE "${INC_DIR}")
            list(APPEND ABS_INCLUDE_DIRS "${INC_DIR}")
        else()
            list(APPEND ABS_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/${INC_DIR}")
        endif()
    endforeach()

    # === Gather all source files ===
    set(SOURCES)
    foreach(SRC_DIR IN LISTS add_module_SOURCE_DIRS)
        file(++++ FOUND_SRC_FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/${SRC_DIR}/*.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/${SRC_DIR}/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/${SRC_DIR}/*.cc"
            "${CMAKE_CURRENT_SOURCE_DIR}/${SRC_DIR}/*.cxx"
        )
        list(APPEND SOURCES ${FOUND_SRC_FILES})
    endforeach()

    # === Gather all header files ===
    set(INCLUDE_SOURCES)
    foreach(INC_DIR IN LISTS ABS_INCLUDE_DIRS)
        file(GLOB FOUND_HEADER_FILES
            "${INC_DIR}/*.h"
            "${INC_DIR}/*.hpp"
            "${INC_DIR}/*.hh"
        )
        list(APPEND INCLUDE_SOURCES ${FOUND_HEADER_FILES})
    endforeach()

    # === Create the object library ===
    add_library("${add_module_NAME}" OBJECT ${SOURCES} ${INCLUDE_SOURCES})

    # === Deduplicate and flatten base dirs ===
    list(SORT ABS_INCLUDE_DIRS)
    set(DEDUP_INCLUDE_DIRS "")
    foreach(DIR IN LISTS ABS_INCLUDE_DIRS)
        set(IS_SUBDIR FALSE)
        foreach(PARENT IN LISTS DEDUP_INCLUDE_DIRS)
            if(DIR MATCHES "^${PARENT}(/|$)")
                set(IS_SUBDIR TRUE)
                break()
            endif()
        endforeach()
        if(NOT IS_SUBDIR)
            list(APPEND DEDUP_INCLUDE_DIRS "${DIR}")
        endif()
    endforeach()

    # === Define header file set for installation ===
    target_sources("${add_module_NAME}" PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        BASE_DIRS ${DEDUP_INCLUDE_DIRS}
        FILES ${INCLUDE_SOURCES}
    )

    message(STATUS "Configuring module ${add_module_NAME}")

    add_library("${PROJECT_PREFIX}::${add_module_NAME}" ALIAS "${add_module_NAME}")

    # === Set include directories with absolute paths ===
    target_include_directories("${add_module_NAME}"
        PUBLIC
            $<BUILD_INTERFACE:${ABS_INCLUDE_DIRS}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    # === Link dependencies ===
    target_link_libraries("${add_module_NAME}"
        PUBLIC
            ${add_module_DEPENDENCIES}
    )

    # === Install target ===
    install(TARGETS ${add_module_NAME}
        EXPORT ${add_module_NAME}Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILE_SET public_headers
    )
endfunction()
