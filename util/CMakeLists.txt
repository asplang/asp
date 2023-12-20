#
# Asp info utility build specification.
#

cmake_minimum_required(VERSION 3.5)

project(aspinfo VERSION 0.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

add_executable(aspinfo
    main.cpp
    )

target_link_libraries(aspinfo
    aspd
    )

target_compile_definitions(aspinfo PRIVATE
    $<$<NOT:$<STREQUAL:${C_COMMAND_OPTION_PREFIXES},>>:
        COMMAND_OPTION_PREFIXES=${C_COMMAND_OPTION_PREFIXES}>
    )

install(TARGETS aspinfo
    DESTINATION bin
    )
