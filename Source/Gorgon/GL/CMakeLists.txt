target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
        ../GL.h
        FrameBuffer.h
        OpenGL.h
        Shader.h
        Simple.h
)

target_sources(Gorgon
    PRIVATE
        FrameBuffer.cpp
        Shader.cpp
)

if(GORGON_GL_OPENGL)
    target_sources(Gorgon
        PRIVATE
        OpenGL.cpp
    )
endif()
