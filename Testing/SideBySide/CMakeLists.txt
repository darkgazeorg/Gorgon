cmake_minimum_required(VERSION 3.25)
project(GorgonSideBySideTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Gorgon REQUIRED)
find_package(Gorgon_vk REQUIRED)

add_executable(test_opengl Main.cpp)
target_link_libraries(test_opengl PRIVATE Gorgon::Gorgon)

add_executable(test_vulkan Main.cpp)
target_link_libraries(test_vulkan PRIVATE Gorgon::Gorgon_vk)
