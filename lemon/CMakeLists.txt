#
# Lemon parser generator build specification.
#

cmake_minimum_required(VERSION 3.5)

project(lemon
    LANGUAGES C
    )

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

add_executable(lemon lemon.c)

# Remove top-level compile options since they are too strict for lemon.
if(WIN32 AND MSVC)
    set_property(TARGET lemon
        PROPERTY COMPILE_OPTIONS "/D_CRT_SECURE_NO_WARNINGS")
else()
    set_property(TARGET lemon
        PROPERTY COMPILE_OPTIONS "")
endif()
