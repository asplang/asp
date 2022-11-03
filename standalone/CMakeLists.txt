#
# Standalone Asp build specification.
#

cmake_minimum_required(VERSION 3.10)

project(asps VERSION 0.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CX_STANDARD_REQUIRED True)

add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/standalone.asps
    DEPENDS ${PROJECT_SOURCE_DIR}/standalone.asps
    COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/standalone.asps ${PROJECT_BINARY_DIR}
    )

add_custom_command(
    OUTPUT
        ${PROJECT_BINARY_DIR}/standalone.aspec
        ${PROJECT_BINARY_DIR}/standalone.c
        ${PROJECT_BINARY_DIR}/standalone.h
    DEPENDS aspg ${PROJECT_BINARY_DIR}/standalone.asps
    COMMAND aspg ${PROJECT_BINARY_DIR}/standalone.asps
    )

add_executable(asps
    main.cpp
    standalone.c
    functions-print.cpp
    )

target_link_libraries(asps
    aspe
    )

target_include_directories(asps PRIVATE
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
    )
