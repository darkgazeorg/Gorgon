cmake_minimum_required(VERSION 3.25)

add_executable(gmm Source/Main.cpp)

target_link_libraries(gmm PRIVATE Gorgon::Gorgon)

set_target_properties(gmm PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/Bin"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

install(TARGETS gmm
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX AND NOT APPLE)
    install(FILES gmm.desktop
        DESTINATION share/applications
    )
    install(FILES gmm-mime.xml
        DESTINATION share/mime/packages
    )

    add_custom_target(gmm-register-file-association
        COMMAND xdg-mime install --novendor "${CMAKE_CURRENT_SOURCE_DIR}/gmm-mime.xml"
        COMMAND xdg-desktop-menu install --novendor "${CMAKE_CURRENT_SOURCE_DIR}/gmm.desktop"
        COMMAND update-mime-database "$ENV{HOME}/.local/share/mime" || true
        COMMENT "Registering .gmm file association"
        VERBATIM
    )
endif()
