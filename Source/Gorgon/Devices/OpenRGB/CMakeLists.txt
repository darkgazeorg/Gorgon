target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
        ../OpenRGB.h
        OpenRGBClient.h
)

target_sources(Gorgon
    PRIVATE
        OpenRGB.cpp
        OpenRGBClient.cpp
)
