#
# Standalone Asp build specification.
#

cmake_minimum_required(VERSION 3.10)

project(asps VERSION 0.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

add_custom_command(
    OUTPUT
        ${PROJECT_BINARY_DIR}/standalone.aspec
        ${PROJECT_BINARY_DIR}/standalone.c
        ${PROJECT_BINARY_DIR}/standalone.h
    DEPENDS
        aspg
        ${PROJECT_SOURCE_DIR}/standalone.asps
        ../engine/math.asps
        ../engine/type.asps
    COMMAND ${CMAKE_COMMAND} -E env
        ASP_SPEC_INCLUDE=${PATH_NAME_SEPARATOR}${aspe_SOURCE_DIR}
        ${aspg_BINARY_DIR}/aspg ${PROJECT_SOURCE_DIR}/standalone.asps
    )

add_executable(asps
    main.cpp
    standalone.c
    functions-print.cpp
    )

target_link_libraries(asps
    aspe
    )

target_compile_definitions(asps PRIVATE
    COMMAND_OPTION_PREFIX="${COMMAND_OPTION_PREFIX}"
    )

target_include_directories(asps PRIVATE
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
    )

install(TARGETS asps
    DESTINATION bin
    )

install(FILES
    ${PROJECT_BINARY_DIR}/standalone.aspec
    DESTINATION etc/asp
    )
