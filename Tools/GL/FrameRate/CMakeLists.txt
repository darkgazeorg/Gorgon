cmake_minimum_required(VERSION 3.25)
project(GorgonFrameRateTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Gorgon QUIET)
find_package(Gorgon_vk QUIET)

if(NOT Gorgon_FOUND AND NOT Gorgon_vk_FOUND)
    message(FATAL_ERROR "Neither Gorgon (OpenGL) nor Gorgon_vk (Vulkan) package was found.")
endif()

if(Gorgon_FOUND)
    add_executable(FrameRateTest_gl Main.cpp)
    target_link_libraries(FrameRateTest_gl PRIVATE Gorgon::Gorgon)
    set_target_properties(FrameRateTest_gl PROPERTIES
        OUTPUT_NAME "FrameRateTest_gl"
        DEBUG_POSTFIX ""
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Bin"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/Bin"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/Bin"
    )
endif()

if(Gorgon_vk_FOUND)
    add_executable(FrameRateTest_vk Main.cpp)
    target_link_libraries(FrameRateTest_vk PRIVATE Gorgon::Gorgon_vk)
    set_target_properties(FrameRateTest_vk PROPERTIES
        OUTPUT_NAME "FrameRateTest_vk"
        DEBUG_POSTFIX ""
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Bin"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/Bin"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/Bin"
    )
endif()
