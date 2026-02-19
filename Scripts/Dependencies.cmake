# --- Graphics backend ------------------------------------------------------
if(GraphicsLibrary STREQUAL "OpenGL")
    set(OPENGL ON CACHE BOOL "Enable OpenGL support" FORCE)
    message(STATUS "Graphics: OpenGL selected -> OPENGL=ON")
else()
    set(OPENGL OFF CACHE BOOL "Enable OpenGL support" FORCE)
endif()

# --- Always-required libraries ---------------------------------------------
find_package(Threads REQUIRED)
target_link_libraries(Gorgon PRIVATE Threads::Threads)

# PNG
find_package(PNG REQUIRED)
target_link_libraries(Gorgon PRIVATE PNG::PNG)
message(STATUS "Found libpng -> ${PNG_VERSION}")

# JPEG
find_package(JPEG REQUIRED)
target_link_libraries(Gorgon PRIVATE JPEG::JPEG)
message(STATUS "Found libjpeg")

# ZLIB
find_package(ZLIB REQUIRED)
target_link_libraries(Gorgon PRIVATE ZLIB::ZLIB)
message(STATUS "Found zlib -> ${ZLIB_VERSION}")

# --- LZMA SDK (7-Zip) ------------------------------------------------------
# Look for the SDK-specific headers (LzmaDec.h) rather than lzma.h
find_path(LZMASDK_INCLUDE_DIR 
    NAMES LzmaDec.h 
    PATH_SUFFIXES lzma-sdk/C lzma-sdk
)

message(${LZMASDK_INCLUDE_DIR})

# Look for the SDK-specific library
find_library(LZMASDK_LIBRARY 
    NAMES lzmasdk lzma-sdk
)

if(LZMASDK_INCLUDE_DIR AND LZMASDK_LIBRARY)
    # Create an imported target so the rest of the build remains clean
    if(NOT TARGET LzmaSdk::LzmaSdk)
        add_library(LzmaSdk::LzmaSdk UNKNOWN IMPORTED)
        set_target_properties(LzmaSdk::LzmaSdk PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${LZMASDK_INCLUDE_DIR}"
            IMPORTED_LOCATION "${LZMASDK_LIBRARY}"
        )
    endif()
    
    target_link_libraries(Gorgon PRIVATE LzmaSdk::LzmaSdk)
    message(STATUS "LZMA SDK: Found (Include: ${LZMASDK_INCLUDE_DIR})")
else()
    message(FATAL_ERROR "LZMA SDK (7-zip) is required but was not found.")
endif()

# PkgConfig (Required for some Linux dependencies)
if(NOT WIN32)
    find_package(PkgConfig REQUIRED)
    if(PkgConfig_FOUND)
        message(STATUS "Found PkgConfig: ${PKG_CONFIG_EXECUTABLE}")
    endif()
endif()

# --- Optional libraries ---------------------------------------------------

# FreeType
if(FreeType)
    find_package(Freetype REQUIRED)
    target_link_libraries(Gorgon PRIVATE Freetype::Freetype)
    message(STATUS "FreeType: SYSTEM")
endif()

# FontConfig (Linux only)
if(NOT WIN32)
    if(FONTCONFIG)
        find_package(Fontconfig REQUIRED)
        
        target_link_libraries(Gorgon PRIVATE Fontconfig::Fontconfig)
        message(STATUS "Fontconfig: SYSTEM")
    else()
        # If explicitly disabled via options
        message(STATUS "Fontconfig: OFF")
    endif()
endif()

# FLAC
if(FLAC)
    find_package(FLAC REQUIRED)
    
    if(TARGET FLAC::FLAC)
        target_link_libraries(Gorgon PRIVATE FLAC::FLAC)
    else()
        target_link_libraries(Gorgon PRIVATE ${FLAC_LIBRARIES})
    endif()
    message(STATUS "FLAC: SYSTEM")
endif()

# OGG
if(OGG)
    find_package(Ogg REQUIRED)
    if(TARGET Ogg::ogg)
        target_link_libraries(Gorgon PRIVATE Ogg::ogg)
    else()
        target_link_libraries(Gorgon PRIVATE ${OGG_LIBRARIES})
    endif()
    message(STATUS "OGG: SYSTEM")
endif()

# Vorbis
if(VORBIS)
    if(WIN32)
        find_package(Vorbis CONFIG REQUIRED)
        target_link_libraries(Gorgon PRIVATE Vorbis::vorbis)
    else()
        pkg_check_modules(VORBIS REQUIRED IMPORTED_TARGET vorbis)
        target_link_libraries(Gorgon PRIVATE PkgConfig::VORBIS)
    endif()
    message(STATUS "Vorbis: SYSTEM")
endif()

# PulseAudio
if(AUDIOLIB STREQUAL "PULSE")
    pkg_check_modules(PULSE REQUIRED IMPORTED_TARGET libpulse)
    target_link_libraries(Gorgon PRIVATE PkgConfig::PULSE)
    message(STATUS "Audio: PulseAudio selected -> PULSEAUDIO=SYSTEM")
endif()

# CURL
if(HTTP)
    if(WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/Source/External/curl/libcurl.lib")
        # Preserved your bundled Windows fallback
        target_link_libraries(Gorgon PRIVATE "${CMAKE_SOURCE_DIR}/Source/External/curl/libcurl.lib" Ws2_32.lib wldap32.lib)
        target_compile_definitions(Gorgon PRIVATE CURL_STATICLIB)
        message(STATUS "CURL: using bundled libcurl")
    else()
        find_package(CURL REQUIRED)
        target_link_libraries(Gorgon PRIVATE CURL::libcurl)
        message(STATUS "CURL: SYSTEM")
    endif()
endif()