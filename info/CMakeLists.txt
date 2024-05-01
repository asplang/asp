#
# Asp info library build specification.
#

cmake_minimum_required(VERSION 3.5)

set(PARENT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

project(aspd
    VERSION 1.1.0.0
    LANGUAGES C
    )

set(ABI_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
if(NOT ${ABI_VERSION} VERSION_EQUAL ${PARENT_VERSION})
    message(FATAL_ERROR
        "${PROJECT_NAME} version not compatible with project version"
        )
endif()
set(TARGET_VERSION ${ABI_VERSION}.${PROJECT_VERSION_PATCH})

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

add_library(aspd
    result.c
    source.c
    )

set_property(TARGET aspd
    PROPERTY VERSION ${TARGET_VERSION}
    )
if(BUILD_SHARED_LIBS)
    set_property(TARGET aspd
        PROPERTY C_VISIBILITY_PRESET hidden
        )
endif()

target_compile_definitions(aspd
    PRIVATE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:USING_SHARED_LIBS>
        ASP_EXPORT_API
    )

target_include_directories(aspd PUBLIC
    "${aspe_SOURCE_DIR}"
    "${aspe_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
    )

if(WIN32)

    install(TARGETS aspd
        RUNTIME
        COMPONENT Runtime
        )

    install(TARGETS aspd
        ARCHIVE
        COMPONENT Development
        )

else()

    if(BUILD_SHARED_LIBS)
        install(TARGETS aspd
            COMPONENT Runtime
            )
    endif()

    if(INSTALL_DEV)

        if(NOT BUILD_SHARED_LIBS)
            install(TARGETS aspd
                DESTINATION lib/asp-${ABI_VERSION}
                COMPONENT Development
                )

            install(
                CODE
                    "execute_process(COMMAND
                    ${CMAKE_COMMAND} -E create_symlink
                    \"asp-${ABI_VERSION}/libaspd.a\"
                    \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libaspd.a\"
                    )"
                COMPONENT Development
                )

        endif()

    endif()

endif()

if(INSTALL_DEV)

    if(WIN32)

        install(FILES asp-info.h
            DESTINATION include
            COMPONENT Development
            )

    else()

        install(FILES asp-info.h
            DESTINATION include/asp-${ABI_VERSION}
            COMPONENT Development
            )

        install(
            CODE
                "execute_process(
                COMMAND ${CMAKE_COMMAND} -E create_symlink
                \"asp-${ABI_VERSION}/asp-info.h\"
                \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/include/asp-info.h\"
                )"
            COMPONENT Development
            )

    endif()

endif()
