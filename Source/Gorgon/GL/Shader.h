/// Contains base class for all shaders ShaderBase
/// and facilities for loading/compiling shaders.

#pragma once

#include <string>
#include <map>
#include "Simple.h"
#include "../Graphics/Color.h"

namespace Gorgon :: GL {

		namespace UBOBindingPoint			{ enum Type { Resolution, }; }
		namespace TextureUnit				{ enum Type { Diffuse, Normal, Specular, Depth, AlphaMask }; }

		unsigned int						SetupUBO(int size, int binding_point);
		void								UpdateUBO(unsigned int ubo, int size, void const * const data);

		class Shader
		{
		protected:
			~Shader();

			Shader(const std::string &name) : name(name), program(0), vertexshader(0), geometryshader(0), fragmentshader(0) {}

			Shader(const Shader&) = delete;

			Shader&operator=(const Shader&) = delete;

			void Use();
			bool IsInitialized();
			int  LocateUniform(const std::string& name);
			int  BindTexture(const std::string &name, int location);
			void UpdateUniform(int name, float value);
			void UpdateUniform(int name, int value);
			void UpdateUniform(int name, const Geometry::Point3D &value);
			void UpdateUniform(int name, const QuadVertices &value);
			void UpdateUniform(int name, const QuadTextureCoords &value);
			void UpdateUniform(int name, const Graphics::RGBAf &value);
			void BindUBO(const std::string& name, UBOBindingPoint::Type bindingPoint);

			void InitializeWithSource(std::string vertexsrc, std::string fragmentsrc,
									  std::string geometrysrc = "", std::map<std::string, std::string> defines={});

			void InitializeWithSource(std::string vertexsrc, std::string fragmentsrc,
									  std::map<std::string, std::string> defines) {
				InitializeWithSource(vertexsrc, fragmentsrc, "", defines);
			}

			void InitializeFromFiles(const std::string& vertexfile, const std::string& fragmentfile,
									 const std::string& geometryfile = "", std::map<std::string, std::string> defines={});

			void InitializeFromFiles(const std::string& vertexfile, const std::string& fragmentfile,
									 std::map<std::string, std::string> defines) {
				InitializeFromFiles(vertexfile, fragmentfile, "", defines);
			}

			std::string name;

		private:
			unsigned int					program;
			unsigned int					vertexshader;
			unsigned int					geometryshader;
			unsigned int					fragmentshader;
		};

	}
