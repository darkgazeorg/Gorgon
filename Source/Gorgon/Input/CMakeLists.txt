target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
		../Input.h
		DnD.h
		Layer.h
		Keyboard.h
		KeyRepeater.h
		Mouse.h
)

target_sources(Gorgon
    PRIVATE
		Input.cpp
		DnD.cpp
		Layer.cpp
		Keyboard.cpp
		KeyRepeater.cpp
)
