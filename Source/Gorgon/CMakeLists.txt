target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
        Animation.h
        Any.h
        Audio.h
        CGI.h
        Config.h
        ConsumableEvent.h
        DataExchange.h
        Enum.h
        Event.h
        Filesystem.h
        GL.h
        Geometry.h
        Graphics.h
        Input.h
        Layer.h
        Main.h
        Multimedia.h
        OS.h
        Property.h
        Resource.h
        SGuid.h
        Scene.h
        Scripting.h
        String.h
        Struct.h
        TMP.h
        Threading.h
        Time.h
        Types.h
        UI.h
        Window.h
        WindowManager.h
)

target_sources(Gorgon
    PRIVATE
        Config.cpp
        Layer.cpp
        Main.cpp
        SGuid.cpp
        Scene.cpp
        Window.cpp
)

if(GSCRIPT_GENERATOR)
    GenerateGscript(Gscripting.cpp)
    target_sources(Gorgon PRIVATE Gscripting.cpp)
endif()

add_subdirectory(Animation)
add_subdirectory(Containers)
add_subdirectory(Encoding)
add_subdirectory(Filesystem)
add_subdirectory(Geometry)
add_subdirectory(GL)
add_subdirectory(Graphics)
add_subdirectory(ImageProcessing)
add_subdirectory(Input)
add_subdirectory(IO)
add_subdirectory(Multimedia)
add_subdirectory(Network)
add_subdirectory(OS)
add_subdirectory(Resource)
add_subdirectory(String)
add_subdirectory(Time)
add_subdirectory(Utils)
add_subdirectory(WindowManager)

if(AUDIO)
    add_subdirectory(Audio)
endif()

if(CGI)
    add_subdirectory(CGI)
endif()

if(SCRIPTING)
    add_subdirectory(Scripting)
endif()

if(UI)
    add_subdirectory(UI)
    add_subdirectory(Widgets)
endif()
