cmake_minimum_required(VERSION 3.10)

project(asp
    # This top-level version identifies the version of the engine ABI.
    # Only major and minor components should be specified.
    # The version of the engine (aspe) and compiler (aspc) projects must be
    # compatible with this version (major and minor components must match).
    VERSION 0.2
    )

option(BUILD_FOR_HOST
    "Enable/disable building tools for the host environment"
    TRUE)
option(BUILD_FOR_TARGET
    "Enable/disable building for the script engine target environment"
    TRUE)
option(BUILD_SHARED_LIBS
    "Enable/disable building of shared libraries"
    FALSE)

add_subdirectory(engine)

if (BUILD_FOR_HOST)
    add_subdirectory(lemon)
    add_subdirectory(compiler)
    add_subdirectory(appspec)
endif()

if (BUILD_FOR_HOST AND BUILD_FOR_TARGET)
    add_subdirectory(standalone)
endif()
