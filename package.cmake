#
# Asp package specifications.
#

set(CPACK_PACKAGE_NAME Asp)
set(CPACK_PACKAGE_VENDOR
    "Canadensys Aerospace Corporation")
set(CPACK_PACKAGE_VERSION_PATCH
    "${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}")
set(CPACK_PACKAGE_DESCRIPTION
    "The Asp Scripting Platform for Embedded Systems")
set(CPACK_PACKAGE_HOMEPAGE_URL
    "https://asplang.org")

set(CPACK_RESOURCE_FILE_LICENSE
    "${PROJECT_SOURCE_DIR}/LICENSE.txt")

set(CPACK_SOURCE_IGNORE_FILES
    "\\\\.swp$;\\\\.git.*;build.*")

if(NOT DEFINED WIN32)
    set(CPACK_SET_DESTDIR TRUE)
endif()

include(CPack)
