target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
		Iterator.h
		../Filesystem.h
)

target_sources(Gorgon
    PRIVATE
    FILES
		Filesystem.cpp
)

if(WIN32)
    target_sources(Gorgon
        PRIVATE
        FILES
            Windows.cpp
    )
else()
    target_sources(Gorgon
        PRIVATE
        FILES
            Linux.cpp
    )
endif()
