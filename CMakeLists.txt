cmake_minimum_required(VERSION 3.25)

include(Scripts/Windows.cmake)

set(CMAKE_CXX_STANDARD 17)

project(Gorgon 
    VERSION 4.1.0
    DESCRIPTION "Gorgon is a cross-platform framework for games and visual applications. \
        It provides a powerful and flexible set of tools for creating high-performance graphics, \
        audio, and input handling. Gorgon is designed to be easy to use and highly customizable."
    LANGUAGES CXX C
)
include(CTest)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/Scripts")

include(CMakeSetup)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_library(Gorgon STATIC "")
add_library(Gorgon::Gorgon ALIAS Gorgon)

include(Options)
include(Compiler)
include(Functions)

add_subdirectory(Tools/ShaderEmbedder)

add_subdirectory(Source/Gorgon)
get_target_property(GORGON_SOURCES Gorgon SOURCES)
get_target_property(GORGON_HEADERS Gorgon HEADER_SET_HEADERS)

if(TESTS)
    include(Tests)
endif()

include(Documentation)
include(Install)

if(EXPORT_BUILD_TREE)
    export(EXPORT GorgonTargets 
           FILE "${CMAKE_CURRENT_BINARY_DIR}/GorgonConfig.cmake"
           NAMESPACE Gorgon::)
           
    export(PACKAGE Gorgon)
    
    message(STATUS "Gorgon Build Tree Exported for Local Discovery")
endif()

target_include_directories(Gorgon PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/Source>
    $<INSTALL_INTERFACE:include> 
)

include(Dependencies)

if(BUILD_EXAMPLES)
    add_subdirectory(Examples)
endif()

if(MSVC)
    include(SourceTree)
endif()
