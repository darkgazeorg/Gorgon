target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
        ../OS.h
)

target_sources(Gorgon
    PRIVATE
        OS.cpp
)

if(WIN32)
    target_sources(Gorgon
        PRIVATE
            Win32.cpp
    )
    target_link_libraries(Gorgon PRIVATE Secur32 Netapi32)
else()
    target_sources(Gorgon
        PRIVATE
            Linux.cpp
    )
endif()
