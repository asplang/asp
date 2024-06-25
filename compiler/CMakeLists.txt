#
# Asp compiler build specification.
#

cmake_minimum_required(VERSION 3.5)

set(PARENT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

file(STRINGS version.txt VERSION)

project(aspc VERSION ${VERSION})

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

add_custom_command(
    OUTPUT
        "${PROJECT_BINARY_DIR}/asp.c"
        "${PROJECT_BINARY_DIR}/asp.h"
        "${PROJECT_BINARY_DIR}/asp.out"
    DEPENDS lemon asp.y
    COMMAND
        "$<TARGET_FILE:lemon>" "-d${PROJECT_BINARY_DIR}"
        "-T${lemon_SOURCE_DIR}/lempar.c"
        "${PROJECT_SOURCE_DIR}/asp.y"
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
    "${aspe_SOURCE_DIR}/integer.c"
    statement.cpp
    function.cpp
    search-path.cpp
    symbol.cpp
    emit.cpp
    executable.cpp
    instruction.cpp
    )

set_property(TARGET aspc
    PROPERTY VERSION ${ABI_VERSION}
    )

target_compile_definitions(aspc PRIVATE
    ASP_COMPILER_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    ASP_COMPILER_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    ASP_COMPILER_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    ASP_COMPILER_VERSION_TWEAK=${PROJECT_VERSION_TWEAK}
    $<$<NOT:$<STREQUAL:${C_PATH_NAME_SEPARATOR},>>:
        PATH_NAME_SEPARATOR=${C_PATH_NAME_SEPARATOR}>
    $<$<NOT:$<STREQUAL:${C_FILE_NAME_SEPARATORS},>>:
        FILE_NAME_SEPARATORS=${C_FILE_NAME_SEPARATORS}>
    $<$<NOT:$<STREQUAL:${C_COMMAND_OPTION_PREFIXES},>>:
        COMMAND_OPTION_PREFIXES=${C_COMMAND_OPTION_PREFIXES}>
    )

target_include_directories(aspc PRIVATE
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
    "${aspe_SOURCE_DIR}"
    )

install(TARGETS aspc
    DESTINATION bin
    COMPONENT Runtime
    )
