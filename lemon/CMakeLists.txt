#
# Lemon parser generator build specification.
#

cmake_minimum_required(VERSION 3.10)

project(lemon)

add_executable(lemon lemon.c)

add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/lempar.c
    COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/lempar.c ${PROJECT_BINARY_DIR}
    )

add_custom_target(lempar ALL
    DEPENDS ${PROJECT_BINARY_DIR}/lempar.c
    )
