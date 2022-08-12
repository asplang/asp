#
# Asp application specification generator build specification.
#

cmake_minimum_required(VERSION 3.10)

project(aspg VERSION 0.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

add_custom_command(
    OUTPUT
        ${PROJECT_SOURCE_DIR}/app.c
        ${PROJECT_SOURCE_DIR}/app.h
        ${PROJECT_SOURCE_DIR}/app.out
    DEPENDS lemon ${PROJECT_SOURCE_DIR}/app.y
    COMMAND lemon app.y
    )

add_executable(aspg
    main.cpp
    generator.cpp
    generator-output.cpp
    lexer.cpp
    grammar.cpp
    app.c
    parameter.cpp
    ../compiler/symbol.cpp
    )

target_include_directories(aspg
    PRIVATE ../compiler
    )
