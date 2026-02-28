cmake_minimum_required(VERSION 3.25)

project(SpaceShooter LANGUAGES CXX)

# --- 1. Source files ---
# You need to list all your source files here so CMake can compile them.
set(APP_SOURCES
    Source/Main.cpp

    Source/Scenes.h

    Source/MainMenu.h

    Source/Game.h
    Source/Game.cpp

    Source/Assets/Player.h
    Source/Assets/Player.cpp
    Source/Assets/Enemy.h
    Source/Assets/Enemy.cpp
    Source/Assets/UI.h
    Source/Assets/UI.cpp

    Source/Mechanics/Game.h
    Source/Mechanics/Game.cpp
    Source/Mechanics/Player.h
    Source/Mechanics/Player.cpp
    Source/Mechanics/Enemy.h
)

# --- 2. GORGON FRAMEWORK ---
if(NOT TARGET Gorgon::Gorgon)
    find_package(Gorgon REQUIRED)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 3. THE EXECUTABLE ---
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_executable(${PROJECT_NAME} WIN32 ${APP_SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE Gorgon::Gorgon)
copy_commands_to_root(${PROJECT_NAME})

# Set output to the local 'Bin' folder for easy access during development
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/Bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/Bin"
)

# --- 4. ASSET MANAGEMENT ---
# This function call will copy the entire "Resources" folder to the output
# directory (Bin) whenever you build the project. This ensures that all your
# assets are available at runtime without needing to manually copy them.
# This function is aware of any changes in resources and will initiate
# copying only when necessary, which speeds up the build process.
gorgon_copy_directory(${PROJECT_NAME} "Resources")

# --- 5. INSTALLATION & PACKAGING ---
#include(GNUInstallDirs)

# install(TARGETS ${PROJECT_NAME}
#     RUNTIME_DEPENDENCY_SET app_deps
#     RUNTIME DESTINATION bin
# )

# install(RUNTIME_DEPENDENCY_SET app_deps
#     DESTINATION bin
# )

# CPack handles the creation of .zip, .tar.gz, or installers
# You can also create full installers. Refer to CPack documentation 
# for more options. Gorgon is dependency free on Windows. On Linux,
# there are some shared libraries that need to be included. CPack will
# automatically detect those and include them in the package.
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "DarkGaze.Org")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_GENERATOR "ZIP;TGZ") # Generates a zip file for easy distribution
include(CPack)
