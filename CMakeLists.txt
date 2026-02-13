cmake_minimum_required(VERSION 3.25)
cmake_policy(VERSION 3.25)

project(Gorgon)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Tools/ShaderEmbedder")

set(GORGON_MAJOR_VERSION 4)
set(GORGON_MINOR_VERSION 0)
set(GORGON_PATCH_VERSION 0)
set(GORGON_VERSION_ALPHA 1)
set(GORGON_VERSION ${GORGON_MAJOR_VERSION}.${GORGON_MINOR_VERSION}.${GORGON_PATCH_VERSION})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/Lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/Lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/Bin)
set(CMAKE_TESTING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/Testing)
set(CMAKE_DOCUMENT_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/Docs)
set(PROJECT_BINARY_DIR ${CMAKE_CURRENT_LIST_DIR}/Bin)
set(PROJECT_PACKAGE_DIR ${CMAKE_CURRENT_LIST_DIR}/Package)

set(gorgon_external_options "" NONE BUILTIN SYSTEM)
set(gorgon_external_mandatory_options "" BUILTIN SYSTEM)

# Enable generation of a compilation database (compile_commands.json) for tooling like clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(OPENGL "Use OpenGL as underlying GL. Should be ON for now." ON)
option(AUDIO "Enable audio." ON)
option(GORGON_FAST_ANY "Make any to work fast by removing empty checks. This will only apply to release mode." OFF)
option(SCRIPTING "Enable scripting module." ON)
option(GSCRIPT_GENERATOR "Create scripting resources." OFF)
option(NO_LOGGING "Disables logger globally. If set, unit tests for logging will fail." OFF)
option(TESTMODE "Enable test mode." OFF)
option(DOCUMENTATION_GRAPHS "Enable graphs in documentation." OFF)
option(TESTS "Enable compiling of test applications." OFF) 
option(CODE_COVERAGE "Enable coverage flags" OFF)

if(WIN32)
    set(PNG BUILTIN CACHE STRING "Use built-in or system PNG library.")
else()
    set(PNG SYSTEM CACHE STRING "Use system or built-in PNG library.")
endif()

if(NOT PNG IN_LIST gorgon_external_mandatory_options)
    message(FATAL_ERROR "PNG should be set to one of ${gorgon_external_options}")
endif()

set_property(CACHE PNG PROPERTY STRINGS ${gorgon_external_mandatory_options})

if(WIN32)
    set(FLAC BUILTIN CACHE STRING "Enable FLAC decoding and streaming.")
else()
    set(FLAC SYSTEM CACHE STRING "Enable FLAC decoding and streaming.")
endif()

if(NOT FLAC IN_LIST gorgon_external_options)
    message(FATAL_ERROR "FLAC should be set to one of ${gorgon_external_options}")
endif()

set_property(CACHE FLAC PROPERTY STRINGS ${gorgon_external_options})

option(UI "Enable UI subsystem." ON)

if(WIN32)
    set(FREETYPE BUILTIN CACHE STRING "Enable FreeType font support")
else()
    set(FREETYPE SYSTEM CACHE STRING "Enable FreeType font support")
endif()

if("${FREETYPE}" STREQUAL "ON")
    set(FREETYPE SYSTEM CACHE STRING "Enable FreeType font support" FORCE)
endif()

if(NOT FREETYPE IN_LIST gorgon_external_options)
    message(FATAL_ERROR "FREETYPE should be set to one of ${gorgon_external_options}")
endif()

set_property(CACHE FREETYPE PROPERTY STRINGS ${gorgon_external_options})

if(WIN32)
    set(VORBIS BUILTIN CACHE STRING "Enable Vorbis audio support")
else()
    set(VORBIS SYSTEM CACHE STRING "Enable Vorbis audio support")
endif()

if(NOT VORBIS IN_LIST gorgon_external_options)
    message(FATAL_ERROR "VORBIS should be set to one of ${gorgon_external_options}")
endif()

set_property(CACHE VORBIS PROPERTY STRINGS ${gorgon_external_options})

if(WIN32)
    set(FONTCONFIG NONE CACHE STRING "Enable Font config support")
else()
    set(FONTCONFIG SYSTEM CACHE STRING "Enable Font config support")
endif()

if("${FONTCONFIG}" STREQUAL "ON")
    set(FONTCONFIG SYSTEM CACHE STRING "Enable FreeType font support" FORCE)
endif()

if(NOT FONTCONFIG IN_LIST gorgon_external_options)
    message(FATAL_ERROR "FONTCONFIG should be set to one of ${gorgon_external_options}")
endif()

set_property(CACHE FONTCONFIG PROPERTY STRINGS ${gorgon_external_options})

OPTION(FORCE_32_BIT "Force 32 bit compilation" OFF)

set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR include CACHE PATH "Installation directory for header files")

if(GSCRIPT_GENERATOR)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Tools/GscriptGenerator")
endif()

set(ADDITIONAL_CLIENT_FLAGS)

if(NOT(${FORCE_32_BIT}))
    set(FIND_LIBRARY_USE_LIB64_PATHS ON)
    if(NOT WIN32)
        list(INSERT CMAKE_SYSTEM_LIBRARY_PATH 0 /usr/lib64)
    endif()
endif()

if(AUDIO AND(NOT AUDIOLIB OR AUDIOLIB STREQUAL ""))
    if(WIN32)
        set(AUDIOLIB WASAPI)
    else()
        set(AUDIOLIB PULSE)
    endif()
endif()

if(AUDIO)
    add_definitions("-DAUDIO")
    add_definitions("-DAUDIO_${AUDIOLIB}")
    string(APPEND ADDITIONAL_CLIENT_FLAGS " -DAUDIO")
    string(APPEND ADDITIONAL_CLIENT_FLAGS " -DAUDIO_${AUDIOLIB}")
endif()

if(FREETYPE STREQUAL "SYSTEM" OR FREETYPE STREQUAL "BUILTIN")
    add_definitions("-DFREETYPE_SUPPORT")
    string(APPEND ADDITIONAL_CLIENT_FLAGS " -DFREETYPE_SUPPORT")
endif()

if(FLAC STREQUAL "SYSTEM" OR FLAC STREQUAL "BUILTIN")
    add_definitions("-DFLAC_SUPPORT")
    string(APPEND ADDITIONAL_CLIENT_FLAGS " -DFLAC_SUPPORT")
endif()

if(VORBIS STREQUAL "SYSTEM" OR VORBIS STREQUAL "BUILTIN")
    add_definitions("-DVORBIS_SUPPORT")
    string(APPEND ADDITIONAL_CLIENT_FLAGS " -DVORBIS_SUPPORT")
endif()

if(FONTCONFIG STREQUAL "SYSTEM" OR FONTCONFIG STREQUAL "BUILTIN")
    add_definitions("-DFONTCONFIG_SUPPORT")
    string(APPEND ADDITIONAL_CLIENT_FLAGS " -DFONTCONFIG_SUPPORT")
endif()

if(SCRIPTING)
    add_definitions("-DSCRIPTING")
    string(APPEND ADDITIONAL_CLIENT_FLAGS " -DSCRIPTING")
endif()

if(OPENGL)
    set(OpenGL_GL_PREFERENCE "LEGACY")
endif()

if(DOCUMENTATION_GRAPHS)
    set(DOXYGRAPH YES)
else()
    set(DOXYGRAPH NO)
endif()

if(TESTMODE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTEST")
endif()

if(CODE_COVERAGE)
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

if(NO_LOGGING)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNO_LOGGING")
endif()

if(GORGON_FAST_ANY)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DGORGON_FAST_ANY")
endif()

if(OPENGL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOPENGL")
endif()

if(UI)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_UI")
endif()

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLINUX -Werror=return-type")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4800")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLM_SWIZZLE -DGLM_FORCE_RADIANS")

foreach(p LIB BIN INCLUDE CMAKE)
    set(var INSTALL_${p}_DIR)
    if(NOT IS_ABSOLUTE "${${var}}")
        set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
    endif()
endforeach()

include(Scripts/Compiler.cmake)
include(Scripts/Macros.cmake)

set(DebugLibs "")
set(OptimizedLibs "")

StartSource(Source/Gorgon)

if(TESTS)
    include(Scripts/Testing.cmake)
    enable_testing()  # Eklenmesi gereken satır
endif()

include(Scripts/Doxygen.cmake)

add_library(Gorgon STATIC ${All})

if(MSVC)
    target_compile_options(Gorgon PRIVATE "/MP")
endif()

set_property(TARGET Gorgon PROPERTY CXX_STANDARD 14)

set_target_properties(Gorgon PROPERTIES
    DEBUG_OUTPUT_NAME Gorgon_d
    RELWITHDEBINFO_OUTPUT_NAME Gorgon_rd
    MINSIZEREL_OUTPUT_NAME Gorgon_min
)

if(NOT ${deps} STREQUAL "")
    add_dependencies(Gorgon ${deps})
endif()

include_directories(Source/External)
include_directories(Source)

include(Scripts/Libs.cmake)

target_link_libraries(Gorgon ${Libs})

if(NOT DebugLibs STREQUAL "")
    target_link_libraries(Gorgon debug ${DebugLibs})
endif()

if(NOT OptimizedLibs STREQUAL "")
    target_link_libraries(Gorgon optimized ${OptimizedLibs})
endif()

install(TARGETS Gorgon
    EXPORT GorgonTargets
    ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
    PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}/Gorgon"
)

export(TARGETS Gorgon APPEND FILE "${PROJECT_PACKAGE_DIR}/GorgonTargets.cmake")
export(PACKAGE Gorgon)

if(WIN32)
    add_definitions("-DDWM")
else()
    add_definitions("-DX11")
endif()

file(RELATIVE_PATH REL_INCLUDE_DIR "${INSTALL_CMAKE_DIR}" "${INSTALL_INCLUDE_DIR}")

set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/Source")

configure_file(Scripts/GorgonConfig.cmake.in
    "${PROJECT_PACKAGE_DIR}/GorgonConfig.cmake" @ONLY)

set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/Source")
configure_file(Scripts/GorgonConfig.cmake.in
    "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/GorgonConfig.cmake" @ONLY)

configure_file(Scripts/GorgonConfigVersion.cmake.in
    "${PROJECT_PACKAGE_DIR}/GorgonConfigVersion.cmake" @ONLY)

install(FILES
    "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/GorgonConfig.cmake"
    "${PROJECT_PACKAGE_DIR}/GorgonConfigVersion.cmake"
    DESTINATION "${INSTALL_CMAKE_DIR}"
)

install(EXPORT GorgonTargets DESTINATION "${INSTALL_CMAKE_DIR}")


if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

if(CODE_COVERAGE AND TESTS)
    find_program(GCOVR_COMMAND gcov)
    if(GCOVR_COMMAND)
        add_custom_target(coverage
            COMMAND ${GCOVR_COMMAND} -r ${CMAKE_SOURCE_DIR} -e ".*External.*" --html --html-details -o coverage.html
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating code coverage report with gcovr..."
        )
    else()
        message(WARNING "gcov not found. The 'coverage' target will not be available.")
    endif()
endif()

# Copy compile_commands.json to the source directory after building the target so editors can find it easily
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
        COMMENT "Copying compile_commands.json to source directory"
    )
endif()
