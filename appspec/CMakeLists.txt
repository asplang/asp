#
# Asp application specification generator build specification.
#

cmake_minimum_required(VERSION 3.5)

set(PARENT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

configure_file(version.txt version.txt COPYONLY) # Force reread on change
file(STRINGS version.txt VERSION)

project(aspg VERSION ${VERSION})

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

add_custom_command(
    OUTPUT
        "${PROJECT_BINARY_DIR}/app.c"
        "${PROJECT_BINARY_DIR}/app.h"
        "${PROJECT_BINARY_DIR}/app.out"
    DEPENDS lemon app.y
    COMMAND
        lemon "-d${PROJECT_BINARY_DIR}"
        "-T${lemon_SOURCE_DIR}/lempar.c"
        $<$<BOOL:${ENABLE_FEATURE_CLASS}>:-DASP_FEATURE_CLASS>
        "${PROJECT_SOURCE_DIR}/app.y"
    )

add_executable(aspg
    main.cpp
    generator.cpp
    generator-output.cpp
    lexer.cpp
    "${aspc_SOURCE_DIR}/lexer-common.cpp"
    "${aspc_SOURCE_DIR}/grammar.cpp"
    "${aspc_SOURCE_DIR}/token.cpp"
    app.c
    literal.cpp
    statement.cpp
    "${aspc_SOURCE_DIR}/function.cpp"
    "${aspc_SOURCE_DIR}/search-path.cpp"
    "${aspc_SOURCE_DIR}/symbol.cpp"
    "${aspc_SOURCE_DIR}/system.cpp"
    crc.c
    )

set_property(TARGET aspg
    PROPERTY VERSION ${ABI_VERSION}
    )

target_compile_definitions(aspg PRIVATE
    ASP_GENERATOR_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    ASP_GENERATOR_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    ASP_GENERATOR_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    ASP_GENERATOR_VERSION_TWEAK=${PROJECT_VERSION_TWEAK}
    $<$<BOOL:${ENABLE_FEATURE_CLASS}>:ASP_FEATURE_CLASS>
    $<$<NOT:$<STREQUAL:${C_PATH_NAME_SEPARATOR},>>:
        PATH_NAME_SEPARATOR=${C_PATH_NAME_SEPARATOR}>
    $<$<NOT:$<STREQUAL:${C_FILE_NAME_SEPARATORS},>>:
        FILE_NAME_SEPARATORS=${C_FILE_NAME_SEPARATORS}>
    $<$<NOT:$<STREQUAL:${C_COMMAND_OPTION_PREFIXES},>>:
        COMMAND_OPTION_PREFIXES=${C_COMMAND_OPTION_PREFIXES}>
    )

target_include_directories(aspg PRIVATE
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
    "${aspc_SOURCE_DIR}"
    "${aspe_BINARY_DIR}"
    "${aspe_SOURCE_DIR}"
    )

target_link_libraries(aspg PRIVATE
    reserved
    )

install(TARGETS aspg
    DESTINATION bin
    COMPONENT Development
    )
