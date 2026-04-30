#
# Standalone Asp build specification.
#

cmake_minimum_required(VERSION 3.5)

set(PARENT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

configure_file(version.txt version.txt COPYONLY) # Force reread on change
file(STRINGS version.txt VERSION)

project(asps VERSION ${VERSION})

set(ABI_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
if(NOT ${ABI_VERSION} VERSION_EQUAL ${PARENT_VERSION})
    message(FATAL_ERROR
        "${PROJECT_NAME} version not compatible with project version"
        )
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

if(ENABLE_FEATURE_CLASS)

    add_custom_command(
        OUTPUT
            "${PROJECT_BINARY_DIR}/standalone-oo.aspec"
            "${PROJECT_BINARY_DIR}/standalone-oo.c"
            "${PROJECT_BINARY_DIR}/standalone-oo.h"
        DEPENDS
            aspg
            "${PROJECT_SOURCE_DIR}/standalone-oo.asps"
            "${aspe_SOURCE_DIR}/sys.asps"
            "${aspe_SOURCE_DIR}/type.asps"
            "${aspe_SOURCE_DIR}/class.asps"
            "${aspe_SOURCE_DIR}/collect.asps"
            "${aspe_SOURCE_DIR}/iter.asps"
            "${aspe_SOURCE_DIR}/math.asps"
        COMMAND
            ${CMAKE_COMMAND} -E env
            "ASP_SPEC_INCLUDE=${PROJECT_SOURCE_DIR}${PATH_NAME_SEPARATOR}${aspe_SOURCE_DIR}"
            "$<TARGET_FILE:aspg>" "-q"
            "${PROJECT_SOURCE_DIR}/standalone-oo.asps"
        )

else()

    add_custom_command(
        OUTPUT
            "${PROJECT_BINARY_DIR}/standalone.aspec"
            "${PROJECT_BINARY_DIR}/standalone.c"
            "${PROJECT_BINARY_DIR}/standalone.h"
        DEPENDS
            aspg
            "${PROJECT_SOURCE_DIR}/standalone.asps"
            "${aspe_SOURCE_DIR}/sys.asps"
            "${aspe_SOURCE_DIR}/type.asps"
            "${aspe_SOURCE_DIR}/collect.asps"
            "${aspe_SOURCE_DIR}/iter.asps"
            "${aspe_SOURCE_DIR}/math.asps"
        COMMAND
            ${CMAKE_COMMAND} -E env
            "ASP_SPEC_INCLUDE=${PROJECT_SOURCE_DIR}${PATH_NAME_SEPARATOR}${aspe_SOURCE_DIR}"
            "$<TARGET_FILE:aspg>" "-q"
            "${PROJECT_SOURCE_DIR}/standalone.asps"
        )

endif()

add_executable(asps
    main.cpp
    $<IF:$<BOOL:${ENABLE_FEATURE_CLASS}>,standalone-oo.c,standalone.c>
    functions-print.cpp
    functions-sleep.cpp
    )

if(ENABLE_DEBUG)
    set_property(TARGET asps
        PROPERTY OUTPUT_NAME asps-d
        )
endif()
set_property(TARGET asps
    PROPERTY VERSION ${ABI_VERSION}
    )

target_compile_definitions(asps PRIVATE
    $<$<BOOL:${ENABLE_DEBUG}>:ASP_DEBUG>
    ASP_STANDALONE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    ASP_STANDALONE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    ASP_STANDALONE_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    ASP_STANDALONE_VERSION_TWEAK=${PROJECT_VERSION_TWEAK}
    $<$<BOOL:${ENABLE_FEATURE_CLASS}>:ASP_FEATURE_CLASS>
    $<$<NOT:$<STREQUAL:${C_COMMAND_OPTION_PREFIXES},>>:
        COMMAND_OPTION_PREFIXES=${C_COMMAND_OPTION_PREFIXES}>
    )

target_include_directories(asps PRIVATE
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
    )

target_link_libraries(asps
    $<IF:$<BOOL:${ENABLE_FEATURE_CLASS}>,aspe-oo,aspe>
    aspm
    aspd
    )

install(TARGETS asps
    DESTINATION bin
    COMPONENT Runtime
    )

if(ENABLE_FEATURE_CLASS)
    set(aspec_name standalone-oo)
else()
    set(aspec_name standalone)
endif()

if(WIN32)

    install(FILES "${PROJECT_BINARY_DIR}/${aspec_name}.aspec"
        DESTINATION etc/asp
        COMPONENT Runtime
        )

else()

    install(FILES "${PROJECT_BINARY_DIR}/${aspec_name}.aspec"
        DESTINATION etc/asp
        RENAME ${aspec_name}-${ABI_VERSION}.aspec
        COMPONENT Runtime
        )

    install(
        CODE
            "execute_process(
            COMMAND ${CMAKE_COMMAND} -E create_symlink
            \"${aspec_name}-${ABI_VERSION}.aspec\"
            \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/etc/asp/${aspec_name}.aspec\"
            )"
        COMPONENT Runtime
        )

endif()
