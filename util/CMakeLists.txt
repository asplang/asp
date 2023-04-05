#
# Asp info utility build specification.
#

cmake_minimum_required(VERSION 3.10)

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
    COMMAND_OPTION_PREFIX=${C_COMMAND_OPTION_PREFIX}
    )

install(TARGETS aspinfo
    DESTINATION bin
    )
