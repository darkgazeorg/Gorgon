# Network module - HTTP files are added only when the HTTP option is enabled
if(HTTP)
    target_sources(Gorgon
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
        FILES
            HTTP.h
    )
    target_sources(Gorgon
        PRIVATE
            HTTP.cpp
    )
endif()
