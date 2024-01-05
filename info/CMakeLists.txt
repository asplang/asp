#
# Asp info library build specification.
#

cmake_minimum_required(VERSION 3.5)

project(aspd
    VERSION 0.1
    LANGUAGES C
    )

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

add_library(aspd
    result.c
    source.c
    )

add_dependencies(aspd
    asp-ver
    )

if(BUILD_SHARED_LIBS)
    set_property(TARGET aspd
        PROPERTY C_VISIBILITY_PRESET hidden)
endif()

target_compile_definitions(aspd
    PRIVATE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:USING_SHARED_LIBS>
        ASP_EXPORT_API
    )

target_include_directories(aspd PUBLIC
    ../engine
    "${aspe_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
    )

install(TARGETS aspd
    LIBRARY
        DESTINATION lib
        COMPONENT Runtime
    ARCHIVE
        DESTINATION lib
        COMPONENT Development
    )

install(FILES
    asp-info.h
    DESTINATION include
    COMPONENT Development
    )
