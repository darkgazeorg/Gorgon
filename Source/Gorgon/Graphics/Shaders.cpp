// ShaderCode.cpp

#include "Shaders.h"


namespace Gorgon :: Graphics {

#include "ShaderSrc.strings.gen"
#include "MaskedShaderSrc.strings.gen"

static std::string ModeName(ShaderMode mode) {
	switch(mode) {
	case ShaderMode::Normal:
		return "Normal";
	case ShaderMode::ToMask:
		return "ToMask";
	default:
		return "Unknown";
	}
}


SimpleShader::SimpleShader(ShaderMode mode) : Shader("Gorgon::Graphics::Simple-"+ModeName(mode)) {
	switch(mode) {
	case ShaderMode::Normal:
		InitializeWithSource(Simple_V, Simple_F);
		break;
	case ShaderMode::ToMask:
		InitializeWithSource(Simple_V, ToMask_F);
		break;
	default:
		throw std::runtime_error("Unknown simple shader mode");
	}
}

MaskedShader::MaskedShader() : Shader("Gorgon::Graphics::Masked") {
	InitializeWithSource(Masked_V, Masked_F);
}

AlphaShader::AlphaShader(ShaderMode mode) : Shader("Gorgon::Graphics::Alpha-"+ModeName(mode)) {
	switch(mode) {
	case ShaderMode::Normal:
		InitializeWithSource(Simple_V, Alpha_F);
		break;
	case ShaderMode::ToMask:
		InitializeWithSource(Simple_V, ToMask_F);
		break;
	default:
		throw std::runtime_error("Unknown alpha shader mode");
	}
}

MaskedAlphaShader::MaskedAlphaShader() : Shader("Gorgon::Graphics::Alpha-Masked") {
	InitializeWithSource(Masked_V, MaskedAlpha_F);
}

FillShader::FillShader(ShaderMode mode) : Shader("Gorgon::Graphics::Fill-"+ModeName(mode)) {
	switch(mode) {
	case ShaderMode::Normal:
		InitializeWithSource(NoTex_V, Fill_F);
		break;
	case ShaderMode::ToMask:
		InitializeWithSource(NoTex_V, ToMaskFill_F);
		break;
	default:
		throw std::runtime_error("Unknown fill shader mode");
	}
}

MaskedFillShader::MaskedFillShader() : Shader("Gorgon::Graphics::Fill-Masked") {
	InitializeWithSource(MaskedNoTex_V, MaskedFill_F);
}

}