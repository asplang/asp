#
# Asp top-level build specification.
#

cmake_minimum_required(VERSION 3.10)

project(asp
    # The engine ABI version (used in compatibility checking) consists of the
    # major and minor components of the top-level version. Therefore, the
    # version of the engine (aspe) and compiler (aspc) projects must be
    # compatible with this version (major and minor components must match).
    VERSION 0.7.2.2
    )

option(BUILD_FOR_HOST
    "Enable/disable building tools for the host environment"
    FALSE)
option(BUILD_FOR_TARGET
    "Enable/disable building for the script engine target environment"
    FALSE)

# If no build options are chosen, build them all.
if(NOT ${BUILD_FOR_HOST} AND NOT ${BUILD_FOR_TARGET})
    set(BUILD_FOR_HOST TRUE)
    set(BUILD_FOR_TARGET TRUE)
endif()

option(BUILD_SHARED_LIBS
    "Enable/disable building of shared libraries"
    FALSE)
option(BUILD_TEST_TARGETS
    "Enable/disable building of tests"
    FALSE)

# Debug options.
option(ENABLE_ENGINE_DEBUG
    "Enable/disable debug output for the engine"
    FALSE)
option(ENABLE_ASPS_DEBUG
    "Enable/disable debug output for the standalone application"
    FALSE)

# Debug output for the standalone application requires debug output for the
# engine.
if(${ENABLE_ASPS_DEBUG})
    set(ENABLE_ENGINE_DEBUG TRUE)
endif()

if(WIN32)
    set(PATH_NAME_SEPARATOR [[;]])
    set(C_PATH_NAME_SEPARATOR [['\\;']])
    set(C_FILE_NAME_SEPARATORS [["\\/"]])
    set(C_COMMAND_OPTION_PREFIXES [["-/"]])

    if(MSVC)
        add_compile_options(/D_CRT_SECURE_NO_WARNINGS)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            # Unsupported option when using clang-cl
            add_compile_options(/Zc:preprocessor)
        endif()
    endif()
else()
    set(PATH_NAME_SEPARATOR :)
    set(C_PATH_NAME_SEPARATOR ':')
    set(C_FILE_NAME_SEPARATORS [["/"]])
    set(C_COMMAND_OPTION_PREFIXES [["-"]])
endif()

add_subdirectory(engine)

if(BUILD_FOR_HOST)
    add_subdirectory(lemon)
    add_subdirectory(compiler)
    add_subdirectory(appspec)
    add_subdirectory(info)
    add_subdirectory(util)
endif()

if(BUILD_FOR_HOST AND BUILD_FOR_TARGET)
    add_subdirectory(standalone)
endif()

if(BUILD_TEST_TARGETS)
    add_subdirectory(test)
endif()
