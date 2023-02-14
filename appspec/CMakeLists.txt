#
# Asp application specification generator build specification.
#

cmake_minimum_required(VERSION 3.10)

project(aspg VERSION 0.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

string(
    COMPARE EQUAL "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}"
    built_in_place)
if(NOT built_in_place)
    add_custom_command(
        OUTPUT "${PROJECT_BINARY_DIR}/app.y"
        DEPENDS "${PROJECT_SOURCE_DIR}/app.y"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${PROJECT_SOURCE_DIR}/app.y" "${PROJECT_BINARY_DIR}"
        )
endif()

add_custom_command(
    OUTPUT
        "${PROJECT_BINARY_DIR}/app.c"
        "${PROJECT_BINARY_DIR}/app.h"
        "${PROJECT_BINARY_DIR}/app.out"
    DEPENDS lemon "${PROJECT_BINARY_DIR}/app.y"
    COMMAND lemon "${PROJECT_BINARY_DIR}/app.y"
    )

add_executable(aspg
    main.cpp
    generator.cpp
    generator-output.cpp
    lexer.cpp
    ../compiler/lexer-common.cpp
    ../compiler/grammar.cpp
    ../compiler/token.cpp
    app.c
    literal.cpp
    statement.cpp
    ../compiler/search-path.cpp
    ../compiler/symbol.cpp
    crc.c
    )

target_compile_definitions(aspg PRIVATE
    PATH_NAME_SEPARATOR='${PATH_NAME_SEPARATOR}'
    FILE_NAME_SEPARATOR='${FILE_NAME_SEPARATOR}'
    )

target_include_directories(aspg PRIVATE
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
    ../compiler
    ../engine
    )

install(TARGETS aspg
    DESTINATION bin
    )
