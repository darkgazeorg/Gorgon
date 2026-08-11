cmake_minimum_required(VERSION 3.25)

project(Gorgon_Examples_UI_Application_ProjectCreator LANGUAGES CXX)

set(CMAKE_CONFIGURATION_TYPES Debug Release)

set(APP_SOURCES
    Source/Main.cpp
)

if(NOT TARGET Gorgon::Gorgon)
    find_package(Gorgon REQUIRED)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME} WIN32 ${APP_SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE Gorgon::Gorgon)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/Bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/Bin"
)
