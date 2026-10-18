target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
        OpenRGBClient.h
)

target_sources(Gorgon
    PRIVATE
        OpenRGBClient.cpp
)
