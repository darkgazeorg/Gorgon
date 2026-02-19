cmake_minimum_required(VERSION 3.25)
project(ShaderEmbedder)

add_executable(ShaderEmbedder Source/Main.cpp)

set_target_properties(ShaderEmbedder PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/Bin"
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

if(MSVC)
    # Replaces ADD_DEFINITIONS
    target_compile_definitions(ShaderEmbedder PRIVATE
        _SBCS
        _CRT_SECURE_NO_WARNINGS
    )

    # Replaces appending /wd4068 to definitions
    target_compile_options(ShaderEmbedder PRIVATE
        /wd4068
    )
endif()
