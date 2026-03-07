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

# ==============================================================================
# 5. Standalone SDK Bundling (Windows / vcpkg only)
# ==============================================================================
# If we are building on Windows using vcpkg, bundle all transitive dependencies
# directly into the Gorgon installation to prevent per-project disk bloat.
# Only do this for Windows when vcpkg is actively being used
if(WIN32 AND DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
    message(STATUS "Bundling vcpkg dependencies into the Gorgon installation...")
    
    # Path to the specific triplet's installed files
    set(VCPKG_ROOT_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")

    # 1. Copy the headers (zlib.h, png.h, etc.)
    install(DIRECTORY "${VCPKG_ROOT_DIR}/include/"
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    # 2. Copy the compiled static libraries (zlib.lib, libpng16.lib, etc.)
    install(DIRECTORY "${VCPKG_ROOT_DIR}/lib/"
            DESTINATION ${CMAKE_INSTALL_LIBDIR})

    # 3. Copy the CMake config files so find_dependency() works without vcpkg
    install(DIRECTORY "${VCPKG_ROOT_DIR}/share/" 
            DESTINATION "share")
            
    # 4. Copy the debug versions of the libraries if they exist (zlibd.lib, etc.)
    if(EXISTS "${VCPKG_ROOT_DIR}/debug/lib/")
        install(DIRECTORY "${VCPKG_ROOT_DIR}/debug/lib/"
                DESTINATION "debug/lib")
    endif()
endif()


# ==============================================================================
# Multi-Config SDK Build & Elevated Installation Target
# ==============================================================================
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(is_multi_config)
    # 1. Define OS-specific elevation commands for the install phase
    if(CMAKE_HOST_WIN32)
        # Windows: Use PowerShell to trigger the UAC Admin prompt
        set(ELEVATE_CMD_REL powershell -NoProfile -Command 
            "Start-Process '${CMAKE_COMMAND}' -ArgumentList '--install', '${CMAKE_BINARY_DIR}', '--config', 'Release' -Verb RunAs -Wait")
        set(ELEVATE_CMD_DBG powershell -NoProfile -Command 
            "Start-Process '${CMAKE_COMMAND}' -ArgumentList '--install', '${CMAKE_BINARY_DIR}', '--config', 'Debug' -Verb RunAs -Wait")
        set(ELEVATE_CMD_RWD powershell -NoProfile -Command 
            "Start-Process '${CMAKE_COMMAND}' -ArgumentList '--install', '${CMAKE_BINARY_DIR}', '--config', 'RelWithDebInfo' -Verb RunAs -Wait")
    else()
        # Linux/macOS: Use PolicyKit for a visual GUI password prompt
        find_program(PKEXEC_CMD pkexec)
        
        # Detect if we are in a GUI environment
        if(DEFINED ENV{DISPLAY} OR DEFINED ENV{WAYLAND_DISPLAY})
            if(PKEXEC_CMD)
                # Modern Linux: Pops up the KDE/GNOME visual password dialog
                set(ELEVATE_CMD_REL ${PKEXEC_CMD} "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --config Release)
                set(ELEVATE_CMD_DBG ${PKEXEC_CMD} "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --config Debug)
                set(ELEVATE_CMD_RWD ${PKEXEC_CMD} "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --config RelWithDebInfo)
            else()
                # Fallback: Spawn a new KDE Konsole window that asks for sudo password
                set(ELEVATE_CMD_REL konsole -e bash -c "sudo \"${CMAKE_COMMAND}\" --install \"${CMAKE_BINARY_DIR}\" --config Release; read -p 'Press Enter to close...'")
                set(ELEVATE_CMD_DBG konsole -e bash -c "sudo \"${CMAKE_COMMAND}\" --install \"${CMAKE_BINARY_DIR}\" --config Debug; read -p 'Press Enter to close...'")
                set(ELEVATE_CMD_RWD konsole -e bash -c "sudo \"${CMAKE_COMMAND}\" --install \"${CMAKE_BINARY_DIR}\" --config RelWithDebInfo; read -p 'Press Enter to close...'")    
            endif()
        else()
            # Non-GUI environment: Use simple sudo
            set(ELEVATE_CMD_REL sudo "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --config Release)
            set(ELEVATE_CMD_DBG sudo "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --config Debug)
            set(ELEVATE_CMD_RWD sudo "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --config RelWithDebInfo)
        endif()
    endif()

    # 2. Create the all-in-one target
    if(MAINTAINER)
        add_custom_target(install_sdk
            # Step A: Force compilation of both configurations first
            COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --config Release
            COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --config Debug
            COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --config RelWithDebInfo
            
            # Step B: Execute the elevated installations
            COMMAND ${ELEVATE_CMD_REL}
            COMMAND ${ELEVATE_CMD_DBG}
            COMMAND ${ELEVATE_CMD_RWD}
            
            COMMENT "Compiling and Elevated-Installing Multi-Config Gorgon SDK..."
        )
    else()
        add_custom_target(install_sdk
            # Step A: Force compilation of both configurations first
            COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --config Release
            COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --config Debug
            
            # Step B: Execute the elevated installations
            COMMAND ${ELEVATE_CMD_REL}
            COMMAND ${ELEVATE_CMD_DBG}
            
            COMMENT "Compiling and Elevated-Installing Multi-Config Gorgon SDK..."
        )
    endif()
endif()