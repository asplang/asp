#
# Asp package specifications.
#

include(InstallRequiredSystemLibraries)

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

set(CPACK_PACKAGE_ICON
    "${PROJECT_SOURCE_DIR}/package\\\\asp-nsis.bmp")
set(CPACK_NSIS_MUI_ICON
    "${PROJECT_SOURCE_DIR}/package\\\\asp.ico")
set(CPACK_NSIS_MUI_UNIICON
    "${PROJECT_SOURCE_DIR}/package\\\\asp.ico")

set(CPACK_PACKAGE_INSTALL_DIRECTORY
    "Asp ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL TRUE)
set(CPACK_NSIS_MODIFY_PATH TRUE)

string(CONCAT STR
    "WriteRegExpandStr HKLM "
    "\\\"SYSTEM\\\\CurrentControlSet\\\\"
    "Control\\\\Session Manager\\\\Environment\\\""
    " ASP_SPEC_FILE "
    "\\\"\$INSTDIR\\\\etc\\\\asp\\\\standalone.aspec\\\"")
list(APPEND CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${STR}")
string(CONCAT STR
    "WriteRegExpandStr HKLM "
    "\\\"SYSTEM\\\\CurrentControlSet\\\\"
    "Control\\\\Session Manager\\\\Environment\\\""
    " ASP_SPEC_INCLUDE "
    "\\\"\$INSTDIR\\\\include\\\\asps\\\"")
list(APPEND CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${STR}")
string(REPLACE ";" "\n" CPACK_NSIS_EXTRA_INSTALL_COMMANDS
    "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}")

string(CONCAT STR
    "DeleteRegValue HKLM "
    "\\\"SYSTEM\\\\CurrentControlSet\\\\"
    "Control\\\\Session Manager\\\\Environment\\\""
    " ASP_SPEC_FILE")
list(APPEND CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "${STR}")
string(CONCAT STR
    "DeleteRegValue HKLM "
    "\\\"SYSTEM\\\\CurrentControlSet\\\\"
    "Control\\\\Session Manager\\\\Environment\\\""
    " ASP_SPEC_INCLUDE")
list(APPEND CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "${STR}")
string(REPLACE ";" "\n" CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS
    "${CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS}")

set(CPACK_NSIS_INSTALLED_ICON_NAME Uninstall.exe)

include(CPack)

string(CONCAT STR
    "The script compiler (aspc), the standalone application (asps), "
    "and the debug info utility (aspinfo)."
    )
cpack_add_component(Runtime
    DISPLAY_NAME "Script writing tools"
    DESCRIPTION ${STR}
    REQUIRED
    )
string(CONCAT STR
    "C header files (*.h), link libraries (*.lib), "
    "and script function specification files (*.asps)."
    )
cpack_add_component(Development
    DISPLAY_NAME "Application development files"
    DESCRIPTION ${STR}
    DISABLED
    DEPENDS Runtime
    )
