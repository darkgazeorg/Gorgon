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

# --- LZMA (xz-utils / liblzma) ----------------------------------------------
find_package(LibLZMA REQUIRED)
target_link_libraries(Gorgon PUBLIC LibLZMA::LibLZMA)
message(STATUS "Found liblzma -> ${LIBLZMA_VERSION_STRING}")

# PkgConfig (Required for some Linux dependencies)
if(NOT WIN32)
    find_package(PkgConfig REQUIRED)
    if(PkgConfig_FOUND)
        message(STATUS "Found PkgConfig: ${PKG_CONFIG_EXECUTABLE}")
    endif()
endif()

# For Utils/Assert
if(WIN32)
    find_package(cpptrace CONFIG REQUIRED)
    target_link_libraries(Gorgon PUBLIC cpptrace::cpptrace)
endif()


# GLEW provides an imported target with all the necessary include paths and libraries
find_package(GLEW REQUIRED)
target_link_libraries(Gorgon PUBLIC GLEW::GLEW)
target_compile_definitions(Gorgon PUBLIC GLEW_STATIC)
message(STATUS "Found GLEW.")

# --- Optional libraries ---------------------------------------------------

# FreeType
if(FreeType)
    find_package(Freetype REQUIRED)
    target_link_libraries(Gorgon PUBLIC Freetype::Freetype)
    message(STATUS "FreeType: SYSTEM")
endif()

# Font enumation
if(WIN32)
    target_link_libraries(Gorgon PUBLIC dwrite)
else()
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
    if(WIN32)
        find_package(FLAC REQUIRED)
    else()
        pkg_check_modules(FLAC REQUIRED flac)
    
        target_include_directories(Gorgon PUBLIC ${FLAC_INCLUDE_DIRS})
    endif()
    
    if(TARGET FLAC::FLAC)
        target_link_libraries(Gorgon PUBLIC FLAC::FLAC)
    else()
        target_link_libraries(Gorgon PUBLIC ${FLAC_LIBRARIES})
    endif()
    message(STATUS "FLAC: SYSTEM")
endif()

# OGG
if(OGG)
    if(WIN32)
        find_package(OGG REQUIRED)
    else()
        pkg_check_modules(OGG REQUIRED ogg)
    
        target_include_directories(Gorgon PUBLIC ${OGG_INCLUDE_DIRS})
    endif()

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
    find_package(CURL REQUIRED)
    target_link_libraries(Gorgon PUBLIC CURL::libcurl)
    message(STATUS "CURL: SYSTEM")
endif()
