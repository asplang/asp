#
# Lemon parser generator build specification.
#

cmake_minimum_required(VERSION 3.10)

project(lemon)

add_executable(lemon lemon.c)

string(
    COMPARE EQUAL "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}"
    built_in_place)
if(NOT built_in_place)
    add_custom_command(
        OUTPUT "${PROJECT_BINARY_DIR}/lempar.c"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${PROJECT_SOURCE_DIR}/lempar.c" "${PROJECT_BINARY_DIR}"
        )
endif()

add_custom_target(lempar ALL
    DEPENDS "${PROJECT_BINARY_DIR}/lempar.c"
    )
