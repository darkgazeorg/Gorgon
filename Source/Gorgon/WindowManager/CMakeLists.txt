target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
        ../WindowManager.h
)

target_sources(Gorgon
    PRIVATE
    FILES
        WindowManager.cpp
)

if(WIN32)
    add_subdirectory(DWM)
else()
    add_subdirectory(X11)
endif()
