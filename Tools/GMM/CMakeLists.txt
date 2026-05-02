cmake_minimum_required(VERSION 3.25)

set(GMM_HELP_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/../../Source/Gorgon/Audio/GMM.md")
set(GMM_HELP_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/GMMHelpText.h")

add_custom_command(
    OUTPUT "${GMM_HELP_OUTPUT}"
    COMMAND ${CMAKE_COMMAND}
        -DINPUT_FILE=${GMM_HELP_INPUT}
        -DOUTPUT_FILE=${GMM_HELP_OUTPUT}
        -P "${CMAKE_CURRENT_SOURCE_DIR}/GenerateHelpText.cmake"
    DEPENDS "${GMM_HELP_INPUT}" "${CMAKE_CURRENT_SOURCE_DIR}/GenerateHelpText.cmake"
    COMMENT "Generating GMM help text from GMM.md"
    VERBATIM
)

set(RESOURCES )

if(WIN32)
    list(APPEND RESOURCES "AppIcon.rc")
endif()

add_executable(gmm 
    Source/Main.cpp 
    "${GMM_HELP_OUTPUT}"
    ${RESOURCES}
)

target_link_libraries(gmm PRIVATE Gorgon::Gorgon)
target_include_directories(gmm PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")

set_target_properties(gmm PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/Bin"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    WIN32_EXECUTABLE $<BOOL:${WIN32}>
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
    install(FILES gmm-icon.png
        DESTINATION share/icons/hicolor/128x128/apps
    )

    add_custom_target(gmm-register-file-association
        COMMAND xdg-mime install --novendor "${CMAKE_CURRENT_SOURCE_DIR}/gmm-mime.xml"
        COMMAND xdg-desktop-menu install --novendor "${CMAKE_CURRENT_SOURCE_DIR}/gmm.desktop"
        COMMAND update-mime-database "$ENV{HOME}/.local/share/mime" || true
        COMMENT "Registering .gmm file association on Linux"
        VERBATIM
    )
endif()

if(WIN32)
    add_custom_target(gmm-register-file-association
        COMMAND reg add "HKCU\\Software\\Classes\\.gmm" /ve /d "GorgonMusicMacro" /f
        COMMAND reg add "HKCU\\Software\\Classes\\GorgonMusicMacro" /ve /d "Gorgon Music Macro File" /f
        COMMAND reg add "HKCU\\Software\\Classes\\GorgonMusicMacro\\shell\\open\\command" /ve /d "\"$<SHELL_PATH:$<TARGET_FILE:gmm>>\" \"--play\" \"%1\"" /f
        COMMENT "Registering .gmm file association on Windows"
        VERBATIM
    )
endif()

gorgon_copy_resource(gmm "gmm-icon.png")
