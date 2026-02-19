set(_GRAPHICS_LIBRARY_CHOICES OpenGL)

# Options
option(USE_DOUBLE "Use double precision for geometry calculations." OFF)
option(FAST_ANY "Enable fast any type. This will disable type safety for any and might cause subtle bugs if used incorrectly." OFF)

# Modules
# Possibly only scripting module can be disabled without breaking the entire build.
set   (GraphicsLibrary "OpenGL" CACHE STRING "Select the graphics library to use")
set_property(CACHE GraphicsLibrary PROPERTY STRINGS ${_GRAPHICS_LIBRARY_CHOICES})
option(AUDIO "Enable audio. Disabling audio might break the entire build." ON)
option(SCRIPTING "Enable scripting module." ON)
option(UI "Enable UI module. Disabling might break the entire build." ON)
option(CGI "Enable CGI module. UI depends on CGI module." ON)

# Optional Features
option(TESTS "Enable compiling of test applications." OFF)
option(MANUAL_TESTS "Compiles manual test applications." OFF)
option(COVERAGE "Enable code coverage instrumentation (only valid in Debug builds)." OFF)
option(DOCUMENTATION "Enable generation of documentation." ON)
option(DOCUMENTATION_GRAPHVIZ "Enable generation of documentation graphs (requires Graphviz)." OFF)
option(BUILD_PDF "Use Doxygen to create the PDF API documentation" OFF)

# Dependencies
#PNG, JPEG, Zlib, LZMA are all forced dependencies. No option to disable them.
set(PNG ON)
set(JPEG ON)
set(ZLIB ON)
set(LZMA ON)

# HTTP (libcurl)
option(HTTP "Enable HTTP data and file transfer." ON)

# Flac, FreeType, Vorbis, and FontConfig are optional dependencies. 
# They can be disabled if the user doesn't want to include them
# FreeType might be required for most use cases, UI might not work without it.
option(FLAC "Enable FLAC audio support." ON)
option(FreeType "Enable FreeType font rendering support. Disabling might break the entire build." ON)
option(OGG "Enable OGG container support." ON)
option(VORBIS "Enable Vorbis audio support." ON)
if(NOT WIN32) 
    option(FONTCONFIG "Enable FontConfig support for font discovery on Linux." ON)
endif()


# Validate selections
if(NOT GraphicsLibrary IN_LIST _GRAPHICS_LIBRARY_CHOICES)
  message(FATAL_ERROR "Invalid value for GraphicsLibrary: ${GraphicsLibrary}\nValid options: ${_GRAPHICS_LIBRARY_CHOICES}")
endif()

if(VORBIS AND NOT OGG)
  message(FATAL_ERROR "Vorbis support requires OGG container support. Please enable OGG.")
endif()

if(COVERAGE AND NOT TESTS)
  message(FATAL_ERROR "Code coverage instrumentation requires tests to be enabled. Please enable TESTS.")
endif()


# Set variables for conditional compilation
if(USE_DOUBLE)
    target_compile_definitions(Gorgon PUBLIC GORGON_USE_DOUBLE)
endif()

if(FAST_ANY)
    target_compile_definitions(Gorgon PUBLIC GORGON_FAST_ANY)
endif()

if(GraphicsLibrary STREQUAL "OpenGL")
    target_compile_definitions(Gorgon PUBLIC GORGON_GL_OPENGL)
endif()

if(AUDIO)
    if(WIN32)
        set(AUDIOLIB WASAPI)
    else()
        set(AUDIOLIB PULSE)
    endif()
    
    target_compile_definitions(Gorgon PUBLIC GORGON_AUDIO_SUPPORT)
    target_compile_definitions(Gorgon PUBLIC GORGON_AUDIO_${AUDIOLIB})
endif()

if(SCRIPTING)
    target_compile_definitions(Gorgon PUBLIC GORGON_SCRIPTING_SUPPORT)
endif()

if(UI)
    target_compile_definitions(Gorgon PUBLIC GORGON_UI_SUPPORT)
endif()

if(FLAC)
    target_compile_definitions(Gorgon PUBLIC GORGON_FLAC_SUPPORT)
endif()

if(FreeType)
    target_compile_definitions(Gorgon PUBLIC GORGON_FREETYPE_SUPPORT)
endif()

if(OGG)
    target_compile_definitions(Gorgon PUBLIC GORGON_OGG_SUPPORT)
endif()

if(VORBIS)
    target_compile_definitions(Gorgon PUBLIC GORGON_VORBIS_SUPPORT)
endif()

if(FONTCONFIG)
    target_compile_definitions(Gorgon PUBLIC GORGON_FONTCONFIG_SUPPORT)
endif()

if(DOCUMENTATION_GRAPHS)
    set(DOXYGRAPH YES)
else()
    set(DOXYGRAPH NO)
endif()

if(TESTS)
    target_compile_definitions(Gorgon PUBLIC GORGON_TESTS)
    target_compile_definitions(Gorgon PRIVATE TEST)
endif()
