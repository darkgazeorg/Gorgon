target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
		../Audio.h
		Basic.h
		Controllers.h
		Environment.h
)

target_sources(Gorgon
    PRIVATE
		Audio.cpp
		Controllers.cpp
		Environment.cpp
)

if(AUDIOLIB STREQUAL "PULSE")
    target_sources(Gorgon
        PRIVATE
            PulseAudio.cpp
    )
endif()

if(AUDIOLIB STREQUAL "WASAPI")
    target_sources(Gorgon
        PRIVATE
            WASAPI.cpp
    )
    target_sources(Gorgon
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
        FILES
            WASAPI.inc.h
    )
endif()
