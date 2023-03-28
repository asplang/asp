#
# Asp compiler build specification.
#

cmake_minimum_required(VERSION 3.10)

set(PARENT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

project(aspc
    VERSION 0.7.1.1
    )

set(ABI_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
if(NOT ${ABI_VERSION} VERSION_EQUAL ${PARENT_VERSION})
    message(FATAL_ERROR
        "${PROJECT_NAME} version not compatible with project version"
        )
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

string(
    COMPARE EQUAL "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}"
    built_in_place)
if(NOT built_in_place)
    add_custom_command(
        OUTPUT "${PROJECT_BINARY_DIR}/asp.y"
        DEPENDS "${PROJECT_SOURCE_DIR}/asp.y"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${PROJECT_SOURCE_DIR}/asp.y" "${PROJECT_BINARY_DIR}"
        )
endif()

add_custom_command(
    OUTPUT
        "${PROJECT_BINARY_DIR}/asp.c"
        "${PROJECT_BINARY_DIR}/asp.h"
        "${PROJECT_BINARY_DIR}/asp.out"
    DEPENDS lemon "${PROJECT_BINARY_DIR}/asp.y"
    COMMAND lemon "${PROJECT_BINARY_DIR}/asp.y"
    )

add_executable(aspc
    main.cpp
    compiler.cpp
    lexer.cpp
    lexer-common.cpp
    grammar.cpp
    token.cpp
    asp.c
    expression.cpp
    statement.cpp
    search-path.cpp
    symbol.cpp
    emit.cpp
    executable.cpp
    instruction.cpp
    )

target_compile_definitions(aspc PRIVATE
    ASP_COMPILER_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    ASP_COMPILER_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    ASP_COMPILER_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    ASP_COMPILER_VERSION_TWEAK=${PROJECT_VERSION_TWEAK}
    PATH_NAME_SEPARATOR='${PATH_NAME_SEPARATOR}'
    FILE_NAME_SEPARATOR='${FILE_NAME_SEPARATOR}'
    COMMAND_OPTION_PREFIX="${COMMAND_OPTION_PREFIX}"
    )

target_include_directories(aspc PRIVATE
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
    ../engine
    )

install(TARGETS aspc
    DESTINATION bin
    )
