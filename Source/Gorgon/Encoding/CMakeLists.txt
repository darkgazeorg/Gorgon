target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
        ../Encoding.h
        JSON.h
        LZMA.h
        PNG.h
        URI.h
        JPEG.h
)

target_sources(Gorgon
    PRIVATE
        Encoding.cpp
        JSON.cpp
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
            Vorbis.cpp
    )
endif()
