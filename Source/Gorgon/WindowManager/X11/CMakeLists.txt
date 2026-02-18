target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
        X11.h
        X11Keysym.h
)

target_sources(Gorgon
    PRIVATE
    FILES
        X11.cpp
        Clipboard.cpp
        DnD.cpp
        Monitor.cpp
        Input.cpp
        Window.cpp
)

find_package(X11 REQUIRED)
include_directories(${X11_INCLUDE_DIR})
target_link_libraries(Gorgon PRIVATE ${X11_LIBRARIES} ${X11_Xrandr_LIB} ${X11_Xinerama_LIB})
