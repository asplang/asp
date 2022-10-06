#
# Asp engine build specification.
#

cmake_minimum_required(VERSION 3.10)

project(aspe VERSION 0.1 LANGUAGES C)

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

    target_compile_definitions(aspe PUBLIC
        $<$<CONFIG:Debug>:ASP_DEBUG>
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
