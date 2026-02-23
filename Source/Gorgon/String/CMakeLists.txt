target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
		../String.h
		AdvancedTextBuilder.h
		Exceptions.h
		Markdown.h
		Tokenizer.h
)

target_sources(Gorgon
    PRIVATE
		String.cpp
		Markdown.cpp
)
