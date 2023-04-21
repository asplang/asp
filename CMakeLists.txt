#
# Asp top-level build specification.
#

cmake_minimum_required(VERSION 3.10)

project(asp
    # The engine ABI version (used in compatibility checking) consists of the
    # major and minor components of the top-level version. Therefore, the
    # version of the engine (aspe) and compiler (aspc) projects must be
    # compatible with this version (major and minor components must match).
    VERSION 0.7.1.5
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

set(PATH_NAME_SEPARATOR "" CACHE STRING
    "Path name separator (leave blank for platform default)")
if("${PATH_NAME_SEPARATOR}" STREQUAL "")
    if(WIN32)
        set(PATH_NAME_SEPARATOR $<SEMICOLON>)
    else()
        set(PATH_NAME_SEPARATOR :)
    endif()
endif()
if(${PATH_NAME_SEPARATOR} STREQUAL $<SEMICOLON>)
    set(C_PATH_NAME_SEPARATOR '\\${PATH_NAME_SEPARATOR}')
else()
    set(C_PATH_NAME_SEPARATOR '${PATH_NAME_SEPARATOR}')
endif()

set(FILE_NAME_SEPARATOR "" CACHE STRING
    "File name separator (leave blank for platform default)")
if("${FILE_NAME_SEPARATOR}" STREQUAL "")
    if(WIN32)
        set(FILE_NAME_SEPARATOR \\)
    else()
        set(FILE_NAME_SEPARATOR /)
    endif()
endif()
if(${FILE_NAME_SEPARATOR} STREQUAL \\)
    set(C_FILE_NAME_SEPARATOR '\\${FILE_NAME_SEPARATOR}')
else()
    set(C_FILE_NAME_SEPARATOR '${FILE_NAME_SEPARATOR}')
endif()

set(COMMAND_OPTION_PREFIX "" CACHE STRING
    "Command line option prefix (leave blank for platform default")
if("${COMMAND_OPTION_PREFIX}" STREQUAL "")
    if(WIN32)
        set(COMMAND_OPTION_PREFIX /)
    else()
        set(COMMAND_OPTION_PREFIX -)
    endif()
endif()
set(C_COMMAND_OPTION_PREFIX \"${COMMAND_OPTION_PREFIX}\")

if(WIN32)
    add_compile_options(/D_CRT_SECURE_NO_WARNINGS /Zc:preprocessor)
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
