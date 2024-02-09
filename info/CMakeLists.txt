#
# Asp info library build specification.
#

cmake_minimum_required(VERSION 3.5)

project(aspd
    VERSION 0.2
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

set_property(TARGET aspd
    PROPERTY VERSION ${PROJECT_VERSION})
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

if(BUILD_SHARED_LIBS)
    install(TARGETS aspd
        DESTINATION lib
        COMPONENT Runtime
        )
endif()

if(INSTALL_DEV)

    if(NOT BUILD_SHARED_LIBS)
        install(TARGETS aspd
            DESTINATION lib
            COMPONENT Development
            )
    endif()

    install(FILES
        asp-info.h
        DESTINATION include
        COMPONENT Development
        )

endif()
