target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
		../Animation.h
		ControlledTimer.h
		Discrete.h
		Instance.h
		Storage.h
)

target_sources(Gorgon
    PRIVATE
		Animation.cpp
)
