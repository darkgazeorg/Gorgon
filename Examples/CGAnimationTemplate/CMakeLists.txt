cmake_minimum_required(VERSION 3.25)

set(PROJECT_NAME "CGA")

project(${PROJECT_NAME})

# Enable generation of a compilation database (compile_commands.json) for tooling like clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Prefer modern CMake-style C++ standard selection
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/Bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/Bin)

find_package(Gorgon REQUIRED)

add_compile_options(-std=c++14)

add_executable(${PROJECT_NAME}
    Source/Main.cpp
    Source/Application.cpp
)

linkgorgon(${PROJECT_NAME})

# Copy compile_commands.json to the source directory after building the target so editors can find it easily
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
        COMMENT "Copying compile_commands.json to source directory"
    )
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Resource/Data.gor ${CMAKE_CURRENT_SOURCE_DIR}/Bin/Data.gor COPYONLY)
