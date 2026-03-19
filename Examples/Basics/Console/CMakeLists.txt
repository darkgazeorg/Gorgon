cmake_minimum_required(VERSION 3.25)

project(ConsoleExample LANGUAGES CXX)

# Only enable Debug and Release builds
set(CMAKE_CONFIGURATION_TYPES Debug Release)

# --- 1. Source files ---
set(APP_SOURCES
    Source/Main.cpp
)

# --- 2. GORGON FRAMEWORK ---
if(NOT TARGET Gorgon::Gorgon)
    find_package(Gorgon REQUIRED)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 3. THE EXECUTABLE ---
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_executable(${PROJECT_NAME} ${APP_SOURCES})
add_compile_definitions(GORGON_USE_CONSOLE)

# Link with the core Gorgon library
target_link_libraries(${PROJECT_NAME} PRIVATE Gorgon::Gorgon)

# Set output to the local 'Bin' folder for easy access during development
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/Bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/Bin"
)
