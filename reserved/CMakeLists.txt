#
# Asp reserved names build specification.
#

cmake_minimum_required(VERSION 3.5)

project(reserved)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

add_library(reserved STATIC
    reserved.c
    )
