target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
        ../GL.h
        FrameBuffer.h
        Shader.h
        Simple.h
)

if(GORGON_GL_OPENGL)
    target_sources(Gorgon
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
        FILES
            OpenGL.h
    )

    target_sources(Gorgon
        PRIVATE
            FrameBuffer.cpp
            Shader.cpp
            OpenGL.cpp
    )
endif()

if(GORGON_GL_VULKAN)
    target_sources(Gorgon
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
        FILES
            Vulkan.h
    )

    target_sources(Gorgon
        PRIVATE
            Vulkan.cpp
            VulkanSurface.cpp
            VulkanUtil.cpp
    )

    # Third party implementations in this file are not subject to the warning rules of Gorgon
    set_source_files_properties(VulkanUtil.cpp
        TARGET_DIRECTORY Gorgon
        PROPERTIES COMPILE_OPTIONS "-w"
    )
endif()
