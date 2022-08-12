#
# Asp compiler build specification.
#

cmake_minimum_required(VERSION 3.10)

project(aspc VERSION 0.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

add_custom_command(
    OUTPUT
        ${PROJECT_SOURCE_DIR}/asp.c 
        ${PROJECT_SOURCE_DIR}/asp.h 
        ${PROJECT_SOURCE_DIR}/asp.out
    DEPENDS lemon ${PROJECT_SOURCE_DIR}/asp.y
    COMMAND lemon asp.y
    )

add_executable(aspc
    main.cpp
    compiler.cpp
    lexer.cpp
    grammar.cpp
    asp.c
    expression.cpp
    statement.cpp
    symbol.cpp
    emit.cpp
    executable.cpp
    instruction.cpp
    )

target_include_directories(aspc PRIVATE
    ../engine
    )
