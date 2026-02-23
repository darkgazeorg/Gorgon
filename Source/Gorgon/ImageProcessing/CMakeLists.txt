target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
      Filters.h
      Kernel.h
)

target_sources(Gorgon
    PRIVATE
		Kernel.cpp
)
