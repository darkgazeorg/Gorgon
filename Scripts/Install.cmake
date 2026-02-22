include(GNUInstallDirs)

install(TARGETS Gorgon
    EXPORT GorgonTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install the "GorgonTargets.cmake" file
install(EXPORT GorgonTargets
    FILE GorgonTargets.cmake
    NAMESPACE Gorgon::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Gorgon
)

# Install a simple Config file (or use a helper to generate one)
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Scripts/GorgonConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/GorgonConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Gorgon
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/GorgonConfigVersion.cmake"
    VERSION ${GORGON_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES 
    "${CMAKE_CURRENT_BINARY_DIR}/GorgonConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/GorgonConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Gorgon
)

#install documentation
if(DOCUMENTATION AND DOXYGEN_FOUND)
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Docs/html/" 
            DESTINATION "share/doc/Gorgon/html")
endif()

set(DOC_INDEX_PATH "${CMAKE_INSTALL_PREFIX}/share/doc/Gorgon/html/index.html")

if(WIN32)
    # Windows: Create a Desktop shortcut (.url) during the install phase
    install(CODE "
        set(SHORTCUT_PATH \"\$ENV{USERPROFILE}/Desktop/Gorgon Documentation.url\")
        file(WRITE \"\${SHORTCUT_PATH}\" 
            \"[InternetShortcut]\\nURL=file:///${DOC_INDEX_PATH}\\nIconIndex=0\\n\")
        message(STATUS \"Created Windows Desktop shortcut: \${SHORTCUT_PATH}\")
    ")
else()
    # Linux: Create a Desktop Entry file
    set(DESKTOP_FILE "${CMAKE_CURRENT_BINARY_DIR}/gorgon-docs.desktop")
    
    # We define the entry with the 'Development' category
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

    # Install the .desktop file to the standard applications menu path
    install(FILES "${DESKTOP_FILE}" 
            DESTINATION "share/applications")
endif()