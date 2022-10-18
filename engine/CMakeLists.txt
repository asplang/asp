#
# Asp engine build specification.
#

cmake_minimum_required(VERSION 3.10)

set(PARENT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

project(aspe
    VERSION 0.1.0.0
    LANGUAGES C
    )

set(ABI_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
if(NOT ${ABI_VERSION} VERSION_EQUAL ${PARENT_VERSION})
    message(FATAL_ERROR
        "${PROJECT_NAME} version not compatible with project version"
        )
endif()

if (BUILD_FOR_TARGET)

    set(CMAKE_C_STANDARD 99)
    set(CMAKE_C_STANDARD_REQUIRED True)

    add_library(aspe
        engine.c
        step.c
        bits.c
        api.c
        data.c
        ref.c
        range.c
        stack.c
        sequence.c
        tree.c
        assign.c
        compare.c
        operation.c
        function.c
        $<$<CONFIG:Debug>:debug.c>
        )

    set_property(TARGET aspe
        PROPERTY VERSION ${ABI_VERSION})

    target_compile_definitions(aspe PUBLIC
        $<$<CONFIG:Debug>:ASP_DEBUG>
        ASP_ENGINE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        ASP_ENGINE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        ASP_ENGINE_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        ASP_ENGINE_VERSION_TWEAK=${PROJECT_VERSION_TWEAK}
        )

    target_include_directories(aspe PUBLIC
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        )

    install(TARGETS aspe
        DESTINATION lib
        )

endif()

if (BUILD_FOR_HOST)

    install(FILES
        asp.h
        asp-priv.h
        DESTINATION include
        )

endif()
