target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
		../Multimedia.h
		AudioStream.h
		Stream.h
		Wave.h
)

target_sources(Gorgon
    PRIVATE
		Multimedia.cpp
		AudioStream.cpp
		Wave.cpp
)
