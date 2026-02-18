# Detect and import system dependencies according to the options from Options.cmake
# - Converts boolean options (FreeType, FLAC, VORBIS, OGG, FONTCONFIG, GraphicsLibrary)
#   into the provider-style variables used by the rest of the project
#   (e.g. FREETYPE, FLAC, VORBIS, OGG, FONTCONFIG, OPENGL).
# - Calls find_package() for required and enabled optional libraries and exposes
#   include directories / library variables for downstream CMake files.

# --- Graphics backend ------------------------------------------------------
if(GraphicsLibrary STREQUAL "OpenGL")
    set(OPENGL ON CACHE BOOL "Enable OpenGL support" FORCE)
    message(STATUS "Graphics: OpenGL selected -> OPENGL=ON")
else()
    set(OPENGL OFF CACHE BOOL "Enable OpenGL support" FORCE)
endif()

# --- Always-required libraries ---------------------------------------------
# libpng, libjpeg and zlib are mandatory in this project (Options.cmake sets them ON)
find_package(Threads REQUIRED)
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(ZLIB REQUIRED)

target_link_libraries(Gorgon PRIVATE Threads::Threads)

if(PNG_FOUND)
    include_directories(${PNG_INCLUDE_DIRS})
    message(STATUS "Found libpng -> ${PNG_VERSION}")
endif()

if(JPEG_FOUND)
    include_directories(${JPEG_INCLUDE_DIRS})
    message(STATUS "Found libjpeg")
endif()

if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    message(STATUS "Found zlib -> ${ZLIB_VERSION}")
endif()

# LZMA is required (liblzma/xz). Always locate the system library and fail if missing.
set(LZMA "SYSTEM" CACHE STRING "LZMA provider" FORCE)

# Try CMake Find module first (if available); otherwise fall back to manual lookup.
find_package(LZMA QUIET)
if(LZMA_FOUND)
    include_directories(${LZMA_INCLUDE_DIRS})
    set(LZMA_LIBRARIES ${LZMA_LIBRARIES} CACHE INTERNAL "")
    message(STATUS "LZMA: SYSTEM (found via FindLZMA)")
else()
    find_path(LZMA_INCLUDE_DIR lzma.h)
    find_library(LZMA_LIBRARY NAMES lzma)

    if(LZMA_INCLUDE_DIR AND LZMA_LIBRARY)
        include_directories(${LZMA_INCLUDE_DIR})
        set(LZMA_INCLUDE_DIRS ${LZMA_INCLUDE_DIR} CACHE INTERNAL "")
        set(LZMA_LIBRARIES ${LZMA_LIBRARY} CACHE INTERNAL "")
        message(STATUS "LZMA: SYSTEM (found lzma.h and liblzma)")
    else()
        message(FATAL_ERROR "LZMA (liblzma/xz) is required but not found. Install the xz / liblzma development package (e.g. liblzma-dev or xz-devel).")
    endif()
endif()

# PkgConfig is required for some dependencies, thus loaded early
if(NOT WIN32)
    find_package(PkgConfig REQUIRED)

    if(PkgConfig_FOUND)
        message(STATUS "Found PkgConfig: ${PKG_CONFIG_EXECUTABLE}")
    else()
        message(FATAL_ERROR "PkgConfig is required but not found. Install pkg-config.")
    endif()
endif()



# --- Optional libraries mapped from boolean options to provider strings -----
# FreeType
if(FreeType)
    set(FREETYPE "SYSTEM" CACHE STRING "FreeType provider" FORCE)
    find_package(Freetype REQUIRED)
    if(Freetype_FOUND)
        # expose include dirs & libs (support both variable name styles)
        include_directories(${Freetype_INCLUDE_DIRS})
        set(FREETYPE_INCLUDE_DIRS ${Freetype_INCLUDE_DIRS} CACHE INTERNAL "")
        set(FREETYPE_LIBRARIES ${Freetype_LIBRARY}${Freetype_LIBRARIES} CACHE INTERNAL "")
        message(STATUS "FreeType: SYSTEM (will compile FreeType-backed code)")
    else()
        message(FATAL_ERROR "FreeType requested but not found. Either install FreeType or disable the FreeType option.")
    endif()
else()
    set(FREETYPE "OFF" CACHE STRING "FreeType provider" FORCE)
endif()

# FontConfig (Linux only)
if(NOT WIN32)
    if(FONTCONFIG)
        set(FONTCONFIG "SYSTEM" CACHE STRING "Fontconfig provider" FORCE)
        # Try to find it early so we can report a clear status message.
        find_package(Fontconfig QUIET)
        if(Fontconfig_FOUND)
            include_directories(${Fontconfig_INCLUDE_DIRS})
            target_link_libraries(Gorgon PRIVATE ${Fontconfig_LIBRARIES})
            message(STATUS "Fontconfig: SYSTEM")
        else()
            message(STATUS "Fontconfig requested: not found on system (you can disable FONTCONFIG option to skip)")
        endif()
    else()
        set(FONTCONFIG "OFF" CACHE STRING "Fontconfig provider" FORCE)
    endif()
endif()

# FLAC
if(FLAC)
    set(FLAC "SYSTEM" CACHE STRING "FLAC provider" FORCE)
    find_package(FLAC REQUIRED)
    if(FLAC_FOUND)
        include_directories(${FLAC_INCLUDE_DIRS})
        message(STATUS "FLAC: SYSTEM")
    else()
        message(FATAL_ERROR "FLAC requested but not found. Install libFLAC or disable FLAC option.")
    endif()
else()
    set(FLAC "OFF" CACHE STRING "FLAC provider" FORCE)
endif()

# OGG / Vorbis
if(OGG)
    set(OGG "SYSTEM" CACHE STRING "OGG provider" FORCE)
    find_package(Ogg REQUIRED)
    if(Ogg_FOUND)
        include_directories(${Ogg_INCLUDE_DIRS})
        message(STATUS "OGG: SYSTEM")
    else()
        message(FATAL_ERROR "OGG requested but not found. Install libogg or disable OGG option.")
    endif()
else()
    set(OGG "OFF" CACHE STRING "OGG provider" FORCE)
endif()

if(VORBIS)
    set(VORBIS "SYSTEM" CACHE STRING "Vorbis provider" FORCE)

    if(WIN32)
        # vcpkg handles this on Windows
        find_package(Vorbis CONFIG REQUIRED)
        set(VORBIS_TARGET Vorbis::vorbis)
    else()
        # Use system PkgConfig on Linux
        pkg_check_modules(VORBIS REQUIRED IMPORTED_TARGET vorbis)
        set(VORBIS_TARGET PkgConfig::VORBIS)
    endif()

    target_link_libraries(Gorgon PRIVATE ${VORBIS_TARGET})

    if(VORBIS_TARGET)
        include_directories(${Vorbis_INCLUDE_DIRS})
        message(STATUS "Vorbis: SYSTEM")
    else()
        message(FATAL_ERROR "Vorbis requested but not found. Install libvorbis (requires OGG) or disable VORBIS option.")
    endif()
else()
    set(VORBIS "OFF" CACHE STRING "Vorbis provider" FORCE)
endif()


# PulseAudio support (if selected as AUDIO backend)
if(AUDIOLIB STREQUAL "PULSE")
    pkg_check_modules(PULSE IMPORTED_TARGET libpulse)
    if(PULSE_FOUND)
        target_link_libraries(Gorgon PRIVATE PkgConfig::PULSE)
        message(STATUS "Audio: PulseAudio selected -> PULSEAUDIO=SYSTEM")
    else()
        message(FATAL_ERROR "PulseAudio requested but not found.")
    endif()
endif()


# CURL / HTTP support (controlled by Options.cmake)
if(HTTP)
    if(WIN32)
        # prefer bundled libcurl on Windows if present
        if(EXISTS "${CMAKE_SOURCE_DIR}/Source/External/curl/libcurl.lib")
            target_link_libraries(Gorgon PRIVATE "${CMAKE_SOURCE_DIR}/Source/External/curl/libcurl.lib" Ws2_32.lib wldap32.lib)
            target_compile_definitions(Gorgon PRIVATE CURL_STATICLIB)
            message(STATUS "CURL: using bundled libcurl")
        else()
            find_package(CURL REQUIRED)
            if(TARGET CURL::libcurl)
                target_link_libraries(Gorgon PRIVATE CURL::libcurl)
            else()
                target_link_libraries(Gorgon PRIVATE ${CURL_LIBRARIES})
            endif()
            message(STATUS "CURL: SYSTEM")
        endif()
    else()
        find_package(CURL REQUIRED)
        if(TARGET CURL::libcurl)
            target_link_libraries(Gorgon PRIVATE CURL::libcurl)
        else()
            target_link_libraries(Gorgon PRIVATE ${CURL_LIBRARIES})
        endif()
        message(STATUS "CURL: SYSTEM")
    endif()
endif()



# --- Final summary ---------------------------------------------------------
message(STATUS "Dependency summary:")
message(STATUS "  OPENGL    = ${OPENGL}")
message(STATUS "  FREETYPE  = ${FREETYPE}")
message(STATUS "  FONTCONFIG= ${FONTCONFIG}")
message(STATUS "  FLAC      = ${FLAC}")
message(STATUS "  OGG       = ${OGG}")
message(STATUS "  VORBIS    = ${VORBIS}")
message(STATUS "  PNG       = ${PNG_FOUND}")
message(STATUS "  JPEG      = ${JPEG_FOUND}")
message(STATUS "  ZLIB      = ${ZLIB_FOUND}")
message(STATUS "  LZMA      = ${LZMA}")
message(STATUS "  CURL      = ${HTTP}")
message(STATUS "  AUDIO     = ${AUDIOLIB}")
