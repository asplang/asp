#
# Asp info library build specification.
#

cmake_minimum_required(VERSION 3.10)

project(aspd
    VERSION 0.1
    LANGUAGES C
    )

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

add_library(aspd
    result.c
    source.c
    )

add_dependencies(aspd
    asp-ver
    )

target_include_directories(aspd PUBLIC
    ../engine
    "${aspe_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
    )

install(FILES
    asp-info.h
    DESTINATION include
    )

install(TARGETS aspd
    DESTINATION lib
    )
