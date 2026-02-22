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
    install(DIRECTORY "${CMAKE_BINARY_DIR}/html/" 
            DESTINATION "share/doc/Gorgon/html")
endif()
