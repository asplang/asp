#
# Asp engine build specification.
#

cmake_minimum_required(VERSION 3.5)

set(PARENT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

project(aspe
    VERSION 0.7.5.1
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

# The template file references the VERSION_VALUE variable.
configure_file(asp-ver-template.h asp-ver.h)

add_custom_target(asp-ver ALL
    DEPENDS "${PROJECT_BINARY_DIR}/asp-ver.h"
    )

if(BUILD_FOR_TARGET)

    set(CMAKE_C_STANDARD 99)
    set(CMAKE_C_STANDARD_REQUIRED True)

    add_library(aspe
        "${PROJECT_BINARY_DIR}/asp-ver.h"
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
        iterator.c
        assign.c
        compare.c
        operation.c
        function.c
        arguments.c
        lib-type.c
        lib-collect.c
        lib-iter.c
        lib-math.c
        $<$<BOOL:${ENABLE_ENGINE_DEBUG}>:debug.c>
        )

    set_property(TARGET aspe
        PROPERTY VERSION ${ABI_VERSION})
    if(BUILD_SHARED_LIBS)
        set_property(TARGET aspe
            PROPERTY C_VISIBILITY_PRESET hidden)
    endif()

    target_compile_definitions(aspe PRIVATE
        $<$<BOOL:${ENABLE_ENGINE_DEBUG}>:ASP_DEBUG>
        $<$<BOOL:${BUILD_TEST_TARGETS}>:ASP_TEST>
        ASP_ENGINE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        ASP_ENGINE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        ASP_ENGINE_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        ASP_ENGINE_VERSION_TWEAK=${PROJECT_VERSION_TWEAK}
        $<$<BOOL:${BUILD_SHARED_LIBS}>:USING_SHARED_LIBS>
        ASP_EXPORT_API ASP_EXPORT_LIB_API
        )

    target_include_directories(aspe PUBLIC
        "${PROJECT_BINARY_DIR}"
        "${PROJECT_SOURCE_DIR}"
        )

    if(WIN32)

        install(TARGETS aspe
            COMPONENT Runtime
            )

    else()

        if(BUILD_SHARED_LIBS)
            install(TARGETS aspe
                DESTINATION lib
                COMPONENT Runtime
                )
        endif()

        if(INSTALL_DEV)

            if(NOT BUILD_SHARED_LIBS)
                install(TARGETS aspe
                    DESTINATION lib/asp-${ABI_VERSION}
                    COMPONENT Development
                    )
            endif()

            # Note: Normally a generator expression would be used to express
            # the library's file name in the CODE text. This is supported in
            # CMake 3.14, but to support older versions, we hard-code the
            # library file name instead.
            if(NOT BUILD_SHARED_LIBS)
                install(
                    CODE
                        "execute_process(COMMAND
                        ${CMAKE_COMMAND} -E create_symlink
                        \"asp-${ABI_VERSION}/libaspe.a\"
                        \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libaspe.a\"
                        )"
                    COMPONENT Development
                    )
            endif()

        endif()

    endif()

endif()

if(INSTALL_DEV)

    if(NOT BUILD_FOR_TARGET)
        add_custom_target(asp_ver_h ALL
            DEPENDS "${PROJECT_BINARY_DIR}/asp-ver.h"
            )
    endif()

    set(HEADER_FILES
        asp.h
        asp-api.h
        asp-priv.h
        "${PROJECT_BINARY_DIR}/asp-ver.h"
        )
    set(ASPS_FILES
        type.asps
        collect.asps
        iter.asps
        math.asps
        )

    if(WIN32)

        install(FILES ${HEADER_FILES}
            DESTINATION include
            COMPONENT Development
            )
        install(FILES ${ASPS_FILES}
            DESTINATION include/asps
            COMPONENT Development
            )

    else()

        install(FILES ${HEADER_FILES}
            DESTINATION include/asp-${ABI_VERSION}
            COMPONENT Development
            )

        foreach(path ${HEADER_FILES})
            get_filename_component(file ${path} NAME)
            install(
                CODE
                    "execute_process(
                    COMMAND ${CMAKE_COMMAND} -E create_symlink
                    \"asp-${ABI_VERSION}/${file}\"
                    \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/include/${file}\"
                    )"
                COMPONENT Development
                )
        endforeach()

        install(FILES ${ASPS_FILES}
            DESTINATION include/asps/${ABI_VERSION}
            COMPONENT Development
            )

        foreach(file ${ASPS_FILES})
            install(
                CODE
                    "execute_process(
                    COMMAND ${CMAKE_COMMAND} -E create_symlink
                    \"${ABI_VERSION}/${file}\"
                    \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/include/asps/${file}\"
                    )"
                COMPONENT Development
                )
        endforeach()

    endif()

endif()
