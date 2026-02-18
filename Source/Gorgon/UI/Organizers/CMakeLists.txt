target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
		Base.h
		Flow.h
		List.h
)

target_sources(Gorgon
    PRIVATE
    FILES
		Base.cpp
		Flow.cpp
		List.cpp
)
