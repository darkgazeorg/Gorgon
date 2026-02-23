cmake_minimum_required(VERSION 3.25)

# Replace GorgonApp with your application name
project(GorgonApp LANGUAGES CXX)

# --- 1. Source files ---
# You can add your source files here.
# You can list them manually (recommended) or use the GLOB approach below.
set(APP_SOURCES
    Source/Main.cpp
)

# --- ALTERNATIVE: GLOB APPROACH ---
# If you prefer to just drop files into the folder and have them "just work":
# file(GLOB_RECURSE APP_SOURCES "Source/*.cpp" "Source/*.h")
# Note: If you add a new file, you must re-run CMake so it "sees" it.

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

# Set output to the local 'Bin' folder for easy access during development
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/Bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/Bin"
)

# --- 4. ASSET MANAGEMENT ---
# Example: Copy a single splash image and the entire textures folder.
# Add your own assets here!
# gorgon_copy_resource(${PROJECT_NAME} "Resources/logo.png")
# gorgon_copy_directory(${PROJECT_NAME} "Resources/Textures")

# --- 5. INSTALLATION & PACKAGING ---
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME} 
    RUNTIME_DEPENDENCY_SET app_deps
    RUNTIME DESTINATION bin
)

install(RUNTIME_DEPENDENCY_SET app_deps
    DESTINATION bin
)

# CPack handles the creation of .zip, .tar.gz, or installers
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "DarkGaze.Org") #Replace this
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_GENERATOR "ZIP;TGZ") # Generates a zip file for easy distribution
include(CPack)
