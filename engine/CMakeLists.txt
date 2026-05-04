#
# Asp engine build specification.
#

cmake_minimum_required(VERSION 3.5)

set(PARENT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

configure_file(version.txt version.txt COPYONLY) # Force reread on change
file(STRINGS version.txt VERSION)

project(aspe
    VERSION ${VERSION}
    LANGUAGES C
    )

set(ABI_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
if(NOT ${ABI_VERSION} VERSION_EQUAL ${PARENT_VERSION})
    message(FATAL_ERROR
        "${PROJECT_NAME} version not compatible with project version"
        )
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

math(EXPR VERSION_VALUE
    "(${aspe_VERSION_MAJOR} << 24) +
     (${aspe_VERSION_MINOR} << 16) +
     (${aspe_VERSION_PATCH} << 8) +
     ${aspe_VERSION_TWEAK}"
    )

# The template file references the VERSION_VALUE variable.
configure_file(asp-ver-template.h asp-ver.h)

add_library(reserved STATIC
    reserved.c
    )

target_compile_definitions(reserved PRIVATE
    $<$<BOOL:${ENABLE_FEATURE_CLASS}>:ASP_FEATURE_CLASS>
    )

target_include_directories(reserved PUBLIC
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    )

if(BUILD_FOR_TARGET)

    set(COMMON_SOURCES
        engine.c
        step.c
        bits.c
        api.c
        code.c
        data.c
        ref.c
        stack.c
        range.c
        string.c
        sequence.c
        search.c
        tree.c
        iterator.c
        assign.c
        compare.c
        operation.c
        function.c
        arguments.c
        lib-sys.c
        lib-type.c
        lib-collect.c
        lib-iter.c
        integer.c
        integer-result.c
        $<$<BOOL:${ENABLE_DEBUG}>:debug.c>
        )

    add_library(aspe
        ${COMMON_SOURCES}
        )

    if(ENABLE_DEBUG)
        set_property(TARGET aspe
            PROPERTY OUTPUT_NAME aspe-d
            )
    endif()
    set_property(TARGET aspe
        PROPERTY VERSION ${ABI_VERSION}
        )
    if(BUILD_SHARED_LIBS)
        set_property(TARGET aspe
            PROPERTY C_VISIBILITY_PRESET hidden
            )
    endif()

    target_compile_definitions(aspe PRIVATE
        $<$<BOOL:${ENABLE_DEBUG}>:ASP_DEBUG>
        $<$<BOOL:${BUILD_FOR_REGRESSION_TEST}>:ASP_DEBUG_NO_PTRS>
        $<$<BOOL:${BUILD_TEST_TARGETS}>:ASP_TEST>
        ASP_ENGINE_VERSION_MAJOR=${aspe_VERSION_MAJOR}
        ASP_ENGINE_VERSION_MINOR=${aspe_VERSION_MINOR}
        ASP_ENGINE_VERSION_PATCH=${aspe_VERSION_PATCH}
        ASP_ENGINE_VERSION_TWEAK=${aspe_VERSION_TWEAK}
        $<$<BOOL:${BUILD_SHARED_LIBS}>:USING_SHARED_LIBS>
        ASP_EXPORT_API ASP_EXPORT_LIB_API
        )

    target_include_directories(aspe PUBLIC
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        )

    if(ENABLE_FEATURE_CLASS)

        add_library(aspe-oo
            ${COMMON_SOURCES}
            lib-class.c
            )

        if(ENABLE_DEBUG)
            set_property(TARGET aspe-oo
                PROPERTY OUTPUT_NAME aspe-oo-d
                )
        endif()
        set_property(TARGET aspe-oo
            PROPERTY VERSION ${ABI_VERSION}
            )
        if(BUILD_SHARED_LIBS)
            set_property(TARGET aspe-oo
                PROPERTY C_VISIBILITY_PRESET hidden
                )
        endif()

        target_compile_definitions(aspe-oo PRIVATE
            $<$<BOOL:${ENABLE_DEBUG}>:ASP_DEBUG>
            $<$<BOOL:${BUILD_FOR_REGRESSION_TEST}>:ASP_DEBUG_NO_PTRS>
            $<$<BOOL:${BUILD_TEST_TARGETS}>:ASP_TEST>
            ASP_ENGINE_VERSION_MAJOR=${aspe_VERSION_MAJOR}
            ASP_ENGINE_VERSION_MINOR=${aspe_VERSION_MINOR}
            ASP_ENGINE_VERSION_PATCH=${aspe_VERSION_PATCH}
            ASP_ENGINE_VERSION_TWEAK=${aspe_VERSION_TWEAK}
            ASP_FEATURE_CLASS
            $<$<BOOL:${BUILD_SHARED_LIBS}>:USING_SHARED_LIBS>
            ASP_EXPORT_API ASP_EXPORT_LIB_API
            )

        target_include_directories(aspe-oo PUBLIC
            "${CMAKE_CURRENT_BINARY_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}"
            )

    endif()

    add_library(aspm
        lib-math.c
        )

    if(ENABLE_DEBUG)
        set_property(TARGET aspm
            PROPERTY OUTPUT_NAME aspm-d
            )
    endif()
    set_property(TARGET aspm
        PROPERTY VERSION ${ABI_VERSION}
        )
    if(BUILD_SHARED_LIBS)
        set_property(TARGET aspm
            PROPERTY C_VISIBILITY_PRESET hidden
            )
    endif()

    target_compile_definitions(aspm PRIVATE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:USING_SHARED_LIBS>
        ASP_EXPORT_API ASP_EXPORT_LIB_API
        )

    target_include_directories(aspm PUBLIC
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        )

    set(LIBS
        aspe
        aspm
        )
    if(ENABLE_FEATURE_CLASS)
        list(APPEND LIBS aspe-oo)
    endif()

    if(WIN32)

        install(TARGETS ${LIBS}
            RUNTIME
            COMPONENT Runtime
            )

        install(TARGETS ${LIBS}
            ARCHIVE
            COMPONENT Development
            )

    else()

        if(BUILD_SHARED_LIBS)
            install(TARGETS ${LIBS}
                DESTINATION lib
                COMPONENT Runtime
                )
        endif()

        if(INSTALL_DEV)

            if(NOT BUILD_SHARED_LIBS)
                install(TARGETS ${LIBS}
                    DESTINATION lib/asp-${ABI_VERSION}
                    COMPONENT Development
                    )
            endif()

            # Note: Normally a generator expression would be used to express
            # the library's file name in the CODE text. This is supported in
            # CMake 3.14, but to support older versions, we hard-code the
            # library file name instead.
            if(NOT BUILD_SHARED_LIBS)
                foreach(lib ${LIBS})
                    install(
                        CODE
                            "execute_process(COMMAND
                            ${CMAKE_COMMAND} -E create_symlink
                            \"asp-${ABI_VERSION}/lib${lib}.a\"
                            \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/lib${lib}.a\"
                            )"
                        COMPONENT Development
                        )
                endforeach()
            endif()

        endif()

    endif()

endif()

if(INSTALL_DEV)

    set(HEADER_FILES
        asp.h
        asp-api.h
        asp-priv.h
        "${CMAKE_CURRENT_BINARY_DIR}/asp-ver.h"
        )
    set(ASPS_FILES
        sys.asps
        type.asps
        collect.asps
        iter.asps
        math.asps
        )
    if(ENABLE_FEATURE_CLASS)
        list(APPEND ASPS_FILES class.asps)
    endif()

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
