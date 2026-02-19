target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
        DWM.h
)

target_sources(Gorgon
    PRIVATE
        DWM.cpp
        Clipboard.cpp
        DnD.cpp
        Monitor.cpp
        Input.cpp
        Window.cpp
)

target_link_libraries(Gorgon PRIVATE Shcore.lib)
