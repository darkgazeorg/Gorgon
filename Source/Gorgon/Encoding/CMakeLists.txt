target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
        LZMA.h
        PNG.h
        URI.h
        JPEG.h
)

target_sources(Gorgon
    PRIVATE
    FILES
        LZMA.cpp
        PNG.cpp
        URI.cpp
        JPEG.cpp
)

if(FLAC)
    target_sources(Gorgon
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
        FILES
            FLAC.h
    )
    target_sources(Gorgon
        PRIVATE
        FILES
            FLAC.cpp
    )
endif()

if(VORBIS)
    target_sources(Gorgon
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
        FILES
            Vorbis.h
    )
    target_sources(Gorgon
        PRIVATE
        FILES
            Vorbis.cpp
    )
endif()
