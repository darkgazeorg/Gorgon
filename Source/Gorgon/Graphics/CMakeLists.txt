target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
		../Graphics.h
		AdvancedPrinter.h
		AdvancedPrinterImpl.h
		AdvancedPrinterConstants.h
		Animations.h
		Bitmap.h
		BlankImage.h
		Color.h
		ColorSpaces.h
		Drawables.h
		EmptyImage.h
		Font.h
		BitmapFont.h
		Layer.h
		Line.h
		MaskedObject.h
		Pointer.h
		Rectangle.h
		ScalableObject.h
		StackedObject.h
		Shaders.h
		Texture.h
		TextureAnimation.h
		TextureTargets.h
		TintedObject.h
		HTMLRenderer.h
)

target_sources(Gorgon
    PRIVATE
    FILES
		AdvancedPrinter.cpp
		Bitmap.cpp
		Color.cpp
		ColorSpaces.cpp
		Font.cpp
		BitmapFont.cpp
		Graphics.cpp
		Layer.cpp
		Line.cpp
		Pointer.cpp
		Rectangle.cpp
		Shaders.cpp
		HTMLRenderer.cpp
)

if(FREETYPE)
    target_sources(Gorgon
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
        FILES
            FreeType.h
    )
    target_sources(Gorgon
        PRIVATE
        FILES
            FreeType.cpp
    )
endif()

EmbedShaders(ShaderSrc.strings.gen Shaders.cpp 
	"Shaders/Simple_V.glsl"
	"Shaders/NoTex_V.glsl"	
	"Shaders/Simple_F.glsl"	
	"Shaders/Alpha_F.glsl"	
	"Shaders/Fill_F.glsl"	
	"Shaders/ToMask_F.glsl"	
	"Shaders/ToMaskFill_F.glsl"	
)

EmbedShaders(MaskedShaderSrc.strings.gen Shaders.cpp 
	"Shaders/Masked_V.glsl"
	"Shaders/MaskedAlpha_F.glsl"	
	"Shaders/MaskedNoTex_V.glsl"	
	"Shaders/MaskedFill_F.glsl"
	"Shaders/Masked_F.glsl"
)
