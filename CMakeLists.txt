cmake_minimum_required(VERSION 3.25)

project(Gorgon 
    VERSION 4.1.0
    DESCRIPTION "Gorgon is a cross-platform framework for games and visual applications. \
        It provides a powerful and flexible set of tools for creating high-performance graphics, \
        audio, and input handling. Gorgon is designed to be easy to use and highly customizable."
    LANGUAGES CXX C
)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/Scripts")

include(CMakeSetup)

add_library(Gorgon STATIC "")

include(Options)
include(Compiler)
include(Install)

add_subdirectory(Source/Gorgon)

target_include_directories(Gorgon PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/Source>
    $<INSTALL_INTERFACE:include> 
)

include(Dependencies)

if(MSVC)
    get_target_property(GORGON_SOURCES Gorgon SOURCES)
    source_group(TREE "${PROJECT_SOURCE_DIR}/Source" FILES ${GORGON_SOURCES})
endif()
