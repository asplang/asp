cmake_minimum_required(VERSION 3.10)

option(BUILD_FOR_HOST
    "Enable/disable building tools for the host environment"
    TRUE)
option(BUILD_FOR_TARGET
    "Enable/disable building for the script engine target environment"
    TRUE)

if (BUILD_FOR_TARGET)
    add_subdirectory(engine)
endif()

if (BUILD_FOR_HOST)
    add_subdirectory(lemon)
    add_subdirectory(compiler)
    add_subdirectory(appspec)
endif()

if (BUILD_FOR_HOST AND BUILD_FOR_TARGET)
    add_subdirectory(standalone)
endif()
