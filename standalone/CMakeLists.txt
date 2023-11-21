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
        "${PROJECT_BINARY_DIR}/standalone.aspec"
        "${PROJECT_BINARY_DIR}/standalone.c"
        "${PROJECT_BINARY_DIR}/standalone.h"
    DEPENDS
        aspg
        "${PROJECT_SOURCE_DIR}/standalone.asps"
        ../engine/type.asps
        ../engine/collect.asps
        ../engine/iter.asps
        ../engine/math.asps
    COMMAND
        ${CMAKE_COMMAND} -E env
        "ASP_SPEC_INCLUDE=${PATH_NAME_SEPARATOR}${aspe_SOURCE_DIR}"
        "$<TARGET_FILE:aspg>"
        "${PROJECT_SOURCE_DIR}/standalone.asps"
    )

add_executable(asps
    main.cpp
    standalone.c
    functions-print.cpp
    functions-sleep.cpp
    )

target_link_libraries(asps
    aspe
    aspd
    )

target_compile_definitions(asps PRIVATE
    $<$<BOOL:${ENABLE_ASPS_DEBUG}>:ASP_DEBUG>
    COMMAND_OPTION_PREFIX=${C_COMMAND_OPTION_PREFIX}
    )

target_include_directories(asps PRIVATE
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
    )

install(TARGETS asps
    DESTINATION bin
    )

install(FILES
    "${PROJECT_BINARY_DIR}/standalone.aspec"
    DESTINATION etc/asp
    )
