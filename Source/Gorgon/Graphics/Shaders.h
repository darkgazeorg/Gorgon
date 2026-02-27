/*
	Actual shader implementations of ShaderBase go here.
*/

#pragma once

#include <string>

#include "../GL/Shader.h"

#include "../GL/OpenGL.h"
#include "Color.h"
#include "../GL.h"


namespace Gorgon :: Graphics {

	enum class ShaderMode {
		Normal,
		ToMask
	};

	class SimpleShader : private GL::Shader
	{
	public:
		static SimpleShader &Use(ShaderMode mode = ShaderMode::Normal) {
			static SimpleShader 
				normal(ShaderMode::Normal),
				tomask(ShaderMode::ToMask)
			;

			switch(mode) {
			case ShaderMode::Normal:
				normal.Shader::Use();
				return normal;
			case ShaderMode::ToMask:
				tomask.Shader::Use();
				return tomask;
			default:
				throw std::runtime_error("Unknown simple shader mode");
			}
		}
		
		SimpleShader &SetVertexCoords(const GL::QuadVertices &value) {
			static int id = LocateUniform("vertex_coords");
			UpdateUniform(id, value);
			
			return *this;
		}
		
		/// Sets texture coordinates
		SimpleShader &SetTextureCoords(const GL::QuadTextureCoords &value) {
			static int id = LocateUniform("tex_coords");
			UpdateUniform(id, value);
			
			return *this;
		}
		
		/// Sets diffuse texture
		SimpleShader &SetDiffuse(GL::Texture value) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, value);
			
			return *this;
		}
		
		SimpleShader &SetTint(const Graphics::RGBAf &value) {
			static int id = LocateUniform("tint");
			UpdateUniform(id, value);
			
			return *this;
		}
		
	private:
		SimpleShader(ShaderMode mode);
	};

	class MaskedShader : private GL::Shader
	{
	public:
		static MaskedShader &Use() {
			static MaskedShader me;

			me.Shader::Use();
			return me;
		}
		
		MaskedShader &SetVertexCoords(const GL::QuadVertices &value) {
			static int id = LocateUniform("vertex_coords");
			UpdateUniform(id, value);
			
			return *this;
		}
		
		/// Sets texture coordinates
		MaskedShader &SetTextureCoords(const GL::QuadTextureCoords &value) {
			static int id = LocateUniform("tex_coords");
			UpdateUniform(id, value);
			
			return *this;
		}
		
		/// Sets diffuse texture
		MaskedShader &SetDiffuse(GL::Texture value) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, value);
			
			return *this;
		}
		
		MaskedShader &SetTint(const Graphics::RGBAf &value) {
			static int id = LocateUniform("tint");
			UpdateUniform(id, value);
			
			return *this;
		}

		MaskedShader &SetMask(GL::Texture value) {
			BindTexture("mask", 1);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, value);

			return *this;
		}

	private:
		MaskedShader();
	};

	class AlphaShader : private GL::Shader
	{
	public:

		static AlphaShader &Use(ShaderMode mode = ShaderMode::Normal) {
			static AlphaShader 
				normal(ShaderMode::Normal),
				tomask(ShaderMode::ToMask);

			switch(mode) {
			case ShaderMode::Normal:
				normal.Shader::Use();
				return normal;
			case ShaderMode::ToMask:
				tomask.Shader::Use();
				return tomask;
			default:
				throw std::runtime_error("Unknown alpha shader mode");
			}
		}
		
		AlphaShader &SetVertexCoords(const GL::QuadVertices &value) {
			static int id = LocateUniform("vertex_coords");
			UpdateUniform(id, value);
			
			return *this;
		}
		
		/// Sets texture coordinates
		AlphaShader &SetTextureCoords(const GL::QuadTextureCoords &value) {
			static int id = LocateUniform("tex_coords");
			UpdateUniform(id, value);
			
			return *this;
		}

		/// Sets alpha texture
		AlphaShader &SetAlpha(GL::Texture value) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, value);

			return *this;
		}

		AlphaShader &SetTint(const Graphics::RGBAf &value) {
			static int id = LocateUniform("tint");
			UpdateUniform(id, value);
			
			return *this;
		}
		
	private:
		AlphaShader(ShaderMode mode);
	};

	class MaskedAlphaShader : private GL::Shader
	{
	public:

		static MaskedAlphaShader &Use() {
			static MaskedAlphaShader me;

			me.Shader::Use();
			return me;
		}
		
		MaskedAlphaShader &SetVertexCoords(const GL::QuadVertices &value) {
			static int id = LocateUniform("vertex_coords");
			UpdateUniform(id, value);
			
			return *this;
		}
		
		/// Sets texture coordinates
		MaskedAlphaShader &SetTextureCoords(const GL::QuadTextureCoords &value) {
			static int id = LocateUniform("tex_coords");
			UpdateUniform(id, value);
			
			return *this;
		}

		/// Sets alpha texture
		MaskedAlphaShader &SetAlpha(GL::Texture value) {
			BindTexture("diffuse", 0);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, value);

			return *this;
		}

		MaskedAlphaShader &SetTint(const Graphics::RGBAf &value) {
			static int id = LocateUniform("tint");
			UpdateUniform(id, value);
			
			return *this;
		}

		MaskedAlphaShader &SetMask(GL::Texture value) {
			BindTexture("mask", 1);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, value);

			return *this;
		}
	private:
		MaskedAlphaShader();
	};

	class FillShader : private GL::Shader {
	public:
		static FillShader &Use(ShaderMode mode = ShaderMode::Normal) {
			static FillShader 
				normal(ShaderMode::Normal),
				tomask(ShaderMode::ToMask);

			switch(mode) {
			case ShaderMode::Normal:
				normal.Shader::Use();
				return normal;
			case ShaderMode::ToMask:
				tomask.Shader::Use();
				return tomask;
			default:
				throw std::runtime_error("Unknown fill shader mode");
			}

		}

		FillShader &SetVertexCoords(const GL::QuadVertices &value) {
			static int id = LocateUniform("vertex_coords");
			UpdateUniform(id, value);

			return *this;
		}

		FillShader &SetTint(const Graphics::RGBAf &value) {
			static int id = LocateUniform("tint");
			UpdateUniform(id, value);

			return *this;
		}

	private:
		FillShader(ShaderMode mode);
	};

	class MaskedFillShader : private GL::Shader {
	public:
		static MaskedFillShader &Use() {
			static MaskedFillShader me;

			me.Shader::Use();
			return me;
		}

		MaskedFillShader &SetVertexCoords(const GL::QuadVertices &value) {
			static int id = LocateUniform("vertex_coords");
			UpdateUniform(id, value);

			return *this;
		}

		MaskedFillShader &SetTint(const Graphics::RGBAf &value) {
			static int id = LocateUniform("tint");
			UpdateUniform(id, value);

			return *this;
		}

		MaskedFillShader &SetMask(GL::Texture value) {
			BindTexture("mask", 1);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, value);

			return *this;
		}

	private:
		MaskedFillShader();
	};


}
