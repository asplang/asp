#
# Asp application specification generator build specification.
#

cmake_minimum_required(VERSION 3.5)

project(aspg VERSION 0.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

add_custom_command(
    OUTPUT
        "${PROJECT_BINARY_DIR}/app.c"
        "${PROJECT_BINARY_DIR}/app.h"
        "${PROJECT_BINARY_DIR}/app.out"
    DEPENDS lemon app.y
    COMMAND
        lemon "-d${PROJECT_BINARY_DIR}"
        "-T${lemon_SOURCE_DIR}/lempar.c"
        "${PROJECT_SOURCE_DIR}/app.y"
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
    $<$<NOT:$<STREQUAL:${C_PATH_NAME_SEPARATOR},>>:
        PATH_NAME_SEPARATOR=${C_PATH_NAME_SEPARATOR}>
    $<$<NOT:$<STREQUAL:${C_FILE_NAME_SEPARATORS},>>:
        FILE_NAME_SEPARATORS=${C_FILE_NAME_SEPARATORS}>
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
