# ==============================================================================
# Gorgon Framework Installation & Export Configuration
# ==============================================================================
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# ==============================================================================
# 1. Main Library Targets & Headers
# ==============================================================================
# Install the compiled binaries, libraries, and public headers to standard paths
install(TARGETS Gorgon
    EXPORT GorgonTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# ==============================================================================
# 2. Documentation & OS Shortcuts
# ==============================================================================
if(DOCUMENTATION AND DOXYGEN_FOUND)
    # Install the generated HTML documentation
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Docs/html/" 
            DESTINATION "share/doc/Gorgon/html")
endif()

set(DOC_INDEX_PATH "${CMAKE_INSTALL_PREFIX}/share/doc/Gorgon/html/index.html")

if(WIN32)
    # Windows: Create a Desktop shortcut (.url) pointing to the installed docs
    install(CODE "
        set(SHORTCUT_PATH \"\$ENV{USERPROFILE}/Desktop/Gorgon Documentation.url\")
        file(WRITE \"\${SHORTCUT_PATH}\" 
            \"[InternetShortcut]\\nURL=file:///${DOC_INDEX_PATH}\\nIconIndex=0\\n\")
        message(STATUS \"Created Windows Desktop shortcut: \${SHORTCUT_PATH}\")
    ")
else()
    # Linux: Create an XDG-compliant Desktop Entry file for application menus
    set(DESKTOP_FILE "${CMAKE_CURRENT_BINARY_DIR}/gorgon-docs.desktop")
    
    file(WRITE "${DESKTOP_FILE}" 
        "[Desktop Entry]\n"
        "Version=1.0\n"
        "Type=Application\n"
        "Name=Gorgon Documentation\n"
        "Comment=Reference for the Gorgon Framework\n"
        "Exec=xdg-open ${DOC_INDEX_PATH}\n"
        "Icon=help-browser\n"
        "Categories=Development;Documentation;\n"
        "Terminal=false\n"
    )

    install(FILES "${DESKTOP_FILE}" DESTINATION "share/applications")
endif()

# ==============================================================================
# 3. Build Tree Export (For Local Development & Testing)
# ==============================================================================
# Allows out-of-tree projects to find Gorgon in the build directory without installing
if(EXPORT_BUILD_TREE)
    # Export the targets file locally
    export(EXPORT GorgonTargets 
           FILE "${CMAKE_CURRENT_BINARY_DIR}/GorgonTargets.cmake"
           NAMESPACE Gorgon::)
           
    # Generate the local config file from the template
    configure_file(
        "${PROJECT_SOURCE_DIR}/Scripts/GorgonConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/GorgonConfig.cmake"
        @ONLY
    )
           
    # Register the package in the CMake user package registry
    export(PACKAGE Gorgon)
    message(STATUS "Gorgon Build Tree Exported for Local Discovery (with Functions)")
endif()

# ==============================================================================
# 4. System Install Export (For Global Usage)
# ==============================================================================
# Defines where the CMake package files will live on the system
set(CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/Gorgon")

# Install the Targets file (maps Gorgon::Gorgon to the installed binaries)
install(EXPORT GorgonTargets
        FILE GorgonTargets.cmake
        NAMESPACE Gorgon::
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}")

# Generate the Config file for the installation tree
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/Scripts/GorgonConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/InstallFiles/GorgonConfig.cmake"
    INSTALL_DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
)

# Generate the Version file to enforce compatibility rules
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/InstallFiles/GorgonConfigVersion.cmake"
    VERSION ${GORGON_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Install the generated Config files AND the custom Public functions script
install(FILES 
    "${CMAKE_CURRENT_BINARY_DIR}/InstallFiles/GorgonConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/InstallFiles/GorgonConfigVersion.cmake"
    "${PROJECT_SOURCE_DIR}/Scripts/Public.cmake"
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
)
