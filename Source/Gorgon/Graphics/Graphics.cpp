#include "../Graphics.h"
#include "../GL/OpenGL.h"
#include "../WindowManager.h"
#include "Layer.h"

namespace Gorgon { namespace Graphics {

	const Geometry::Pointf TextureSource::fullcoordinates[4]={{0, 0}, {1, 0}, {1, 1}, {0, 1}};

	namespace internal {

		GL::Texture LastTexture=0;

		int quadvertexindex[] ={
			0, 3, 1,
			1, 3, 2
		};

		GLuint quadvbo;
		std::map<decltype(WindowManager::CurrentContext()), GLuint> vaos;

		void ActivateQuadVertices() {
#ifndef NDEBUG
			if(vaos.count(WindowManager::CurrentContext())==0) {
				throw std::logic_error("Context not initialized???");
			}
#endif
			glBindVertexArray(vaos[WindowManager::CurrentContext()]);
		}

		void DrawQuadVertices() {
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
	}

	void Initialize() {
		using namespace internal;

		static bool initialized=false;
		if(!initialized) {
			initialized=true;
			GL::LoadFunctions();

			glGenBuffers(1, &quadvbo);

			glBindBuffer(GL_ARRAY_BUFFER, quadvbo);
			glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(int), quadvertexindex, GL_STATIC_DRAW);

			Layer::mask.Generate(true);
		}

		if(vaos.count(WindowManager::CurrentContext())==0) {
			auto ctx=WindowManager::CurrentContext();
			glGenVertexArrays(1, &vaos[ctx]);
			glBindVertexArray(vaos[ctx]);

			glEnableVertexAttribArray(0);
			glVertexAttribIPointer(0, 1, GL_INT, 0, (GLvoid*)0);
		}
		
	}

} }
