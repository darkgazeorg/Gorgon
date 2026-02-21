# --- Graphics backend ------------------------------------------------------
if(GraphicsLibrary STREQUAL "OpenGL")
    set(OPENGL ON CACHE BOOL "Enable OpenGL support" FORCE)
    message(STATUS "Graphics: OpenGL selected -> OPENGL=ON")
else()
    set(OPENGL OFF CACHE BOOL "Enable OpenGL support" FORCE)
endif()

# --- Always-required libraries ---------------------------------------------
find_package(Threads REQUIRED)
target_link_libraries(Gorgon PUBLIC Threads::Threads)

# PNG
find_package(PNG REQUIRED)
target_link_libraries(Gorgon PUBLIC PNG::PNG)
message(STATUS "Found libpng -> ${PNG_VERSION}")

# JPEG
find_package(JPEG REQUIRED)
target_link_libraries(Gorgon PUBLIC JPEG::JPEG)
message(STATUS "Found libjpeg")

# ZLIB
find_package(ZLIB REQUIRED)
target_link_libraries(Gorgon PUBLIC ZLIB::ZLIB)
message(STATUS "Found zlib -> ${ZLIB_VERSION}")

# --- LZMA SDK (7-Zip) ------------------------------------------------------
# Look for the SDK-specific headers (LzmaDec.h) rather than lzma.h
find_path(LZMASDK_INCLUDE_DIR 
    NAMES LzmaDec.h 
    PATH_SUFFIXES lzma-sdk/C lzma-sdk
)

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
    
    target_link_libraries(Gorgon PUBLIC LzmaSdk::LzmaSdk)
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
    target_link_libraries(Gorgon PUBLIC Freetype::Freetype)
    message(STATUS "FreeType: SYSTEM")
endif()

# FontConfig (Linux only)
if(NOT WIN32)
    if(FONTCONFIG)
        find_package(Fontconfig REQUIRED)
        
        target_link_libraries(Gorgon PUBLIC Fontconfig::Fontconfig)
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
        target_link_libraries(Gorgon PUBLIC FLAC::FLAC)
    else()
        target_link_libraries(Gorgon PUBLIC ${FLAC_LIBRARIES})
    endif()
    message(STATUS "FLAC: SYSTEM")
endif()

# OGG
if(OGG)
    find_package(Ogg REQUIRED)
    if(TARGET Ogg::ogg)
        target_link_libraries(Gorgon PUBLIC Ogg::ogg)
    else()
        target_link_libraries(Gorgon PUBLIC ${OGG_LIBRARIES})
    endif()
    message(STATUS "OGG: SYSTEM")
endif()

# Vorbis
if(VORBIS)
    if(WIN32)
        # On Windows/vcpkg, the CONFIG package usually provides these targets
        find_package(Vorbis CONFIG REQUIRED)
        # Link both the base and the file API
        target_link_libraries(Gorgon PUBLIC Vorbis::vorbis Vorbis::vorbisfile)
    else()
        # On Linux, you must explicitly ask for 'vorbisfile' in pkg_check_modules
        pkg_check_modules(VORBIS REQUIRED IMPORTED_TARGET vorbis vorbisfile)
        target_link_libraries(Gorgon PUBLIC PkgConfig::VORBIS)
    endif()
    message(STATUS "Vorbis: SYSTEM (including vorbisfile)")
endif()

# PulseAudio
if(AUDIOLIB STREQUAL "PULSE")
    pkg_check_modules(PULSE REQUIRED IMPORTED_TARGET libpulse)
    target_link_libraries(Gorgon PUBLIC PkgConfig::PULSE)
    message(STATUS "Audio: PulseAudio selected -> PULSEAUDIO=SYSTEM")
endif()

# CURL
if(HTTP)
    if(WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/Source/External/curl/libcurl.lib")
        # Preserved your bundled Windows fallback
        target_link_libraries(Gorgon PUBLIC "${CMAKE_SOURCE_DIR}/Source/External/curl/libcurl.lib" Ws2_32.lib wldap32.lib)
        target_compile_definitions(Gorgon PUBLIC CURL_STATICLIB)
        message(STATUS "CURL: using bundled libcurl")
    else()
        find_package(CURL REQUIRED)
        target_link_libraries(Gorgon PUBLIC CURL::libcurl)
        message(STATUS "CURL: SYSTEM")
    endif()
endif()
