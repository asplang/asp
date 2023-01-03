#
# Asp engine build specification.
#

cmake_minimum_required(VERSION 3.10)

set(PARENT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

project(aspe
    VERSION 0.6.0.0
    LANGUAGES C
    )

set(ABI_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
if(NOT ${ABI_VERSION} VERSION_EQUAL ${PARENT_VERSION})
    message(FATAL_ERROR
        "${PROJECT_NAME} version not compatible with project version"
        )
endif()

math(EXPR VERSION_VALUE
    "(${PROJECT_VERSION_MAJOR} << 24) +
     (${PROJECT_VERSION_MINOR} << 16) +
     (${PROJECT_VERSION_PATCH} << 8) +
     ${PROJECT_VERSION_TWEAK}"
    )

add_custom_command(
    OUTPUT asp-ver.h
    COMMAND ${CMAKE_COMMAND} -E echo "/* Asp engine version. */"
        > ${PROJECT_BINARY_DIR}/asp-ver.h
    COMMAND ${CMAKE_COMMAND} -E echo "\\#ifndef ASP_VERSION"
        >> ${PROJECT_BINARY_DIR}/asp-ver.h
    COMMAND ${CMAKE_COMMAND} -E echo "\\#define ASP_VERSION ${VERSION_VALUE}"
        >> ${PROJECT_BINARY_DIR}/asp-ver.h
    COMMAND ${CMAKE_COMMAND} -E echo "\\#endif"
        >> ${PROJECT_BINARY_DIR}/asp-ver.h
    )

if(BUILD_FOR_TARGET)

    set(CMAKE_C_STANDARD 99)
    set(CMAKE_C_STANDARD_REQUIRED True)

    add_library(aspe
        asp-ver.h
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
        arguments.c
        lib-type.c
        lib-math.c
        $<$<CONFIG:Debug>:debug.c>
        )

    set_property(TARGET aspe
        PROPERTY VERSION ${ABI_VERSION})

    target_compile_definitions(aspe PUBLIC
        $<$<CONFIG:Debug>:ASP_DEBUG>
        $<$<BOOL:${BUILD_TEST_TARGETS}>:ASP_TEST>
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

if(BUILD_FOR_HOST)

    if(NOT BUILD_FOR_TARGET)
        add_custom_target(asp_ver_h ALL
            DEPENDS ${PROJECT_BINARY_DIR}/asp-ver.h
            )
    endif()

    install(FILES
        asp.h
        asp-priv.h
        ${PROJECT_BINARY_DIR}/asp-ver.h
        DESTINATION include
        )

    install(FILES
        type.asps
        math.asps
        DESTINATION include/asps
        )

endif()
