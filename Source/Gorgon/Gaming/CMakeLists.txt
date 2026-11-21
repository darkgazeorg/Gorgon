target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
		../Graphics/Layer.h
		../Graphics/Shaders.h
        Layer.h
)

target_sources(Gorgon
    PRIVATE
		Layer.cpp
)