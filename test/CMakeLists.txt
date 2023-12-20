#
# Asp testing build specification.
#

cmake_minimum_required(VERSION 3.5)

project(test VERSION 0.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

add_executable(test-tree
    main-test-tree.cpp
    )

target_link_libraries(test-tree
    aspe
    )

target_compile_definitions(test-tree PRIVATE
    ASP_TEST
    )
