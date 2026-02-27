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
        Time.cpp
)

if(WIN32)
    target_sources(Gorgon
        PRIVATE
            Windows.cpp
    )
    target_link_libraries(Gorgon PRIVATE winmm.lib)
else()
    target_sources(Gorgon
        PRIVATE
            Linux.cpp
    )
endif()
