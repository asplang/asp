#
# Standalone Asp build specification.
#

cmake_minimum_required(VERSION 3.5)

project(asps VERSION 0.2.0.1)

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
        "${aspe_SOURCE_DIR}/type.asps"
        "${aspe_SOURCE_DIR}/collect.asps"
        "${aspe_SOURCE_DIR}/iter.asps"
        "${aspe_SOURCE_DIR}/math.asps"
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
    $<$<NOT:$<STREQUAL:${C_COMMAND_OPTION_PREFIXES},>>:
        COMMAND_OPTION_PREFIXES=${C_COMMAND_OPTION_PREFIXES}>
    )

target_include_directories(asps PRIVATE
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
    )

install(TARGETS asps
    DESTINATION bin
    COMPONENT Runtime
    )

install(FILES
    "${PROJECT_BINARY_DIR}/standalone.aspec"
    DESTINATION etc/asp
    COMPONENT Runtime
    )
