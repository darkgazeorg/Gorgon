target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
        ../Time.h
        Timer.h
)

target_sources(Gorgon
    PRIVATE
    FILES
        Time.cpp
)

if(WIN32)
    target_sources(Gorgon
        PRIVATE
        FILES
            Windows.cpp
    )
    target_link_libraries(Gorgon PRIVATE winmm.lib)
else()
    target_sources(Gorgon
        PRIVATE
        FILES
            Linux.cpp
    )
endif()
