#
# Lemon parser generator build specification.
#

cmake_minimum_required(VERSION 3.5)

project(lemon
    LANGUAGES C
    )

add_executable(lemon lemon.c)
