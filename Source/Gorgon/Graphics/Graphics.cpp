#include "../Graphics.h"
#include "Layer.h"

namespace Gorgon :: Graphics {

	const Geometry::Pointf TextureSource::fullcoordinates[4]={{0, 0}, {1, 0}, {1, 1}, {0, 1}};

	namespace internal {

		GL::Texture LastTexture=0;

		void ActivateQuadVertices() {
			GL::ActivateQuadVertices();
		}

		void DrawQuadVertices() {
			GL::DrawQuadVertices();
		}
	}

	void Initialize() {
		static bool initialized=false;
		if(!initialized) {
			initialized=true;
			GL::Initialize();

			Layer::mask.Generate(true);
		}

		GL::SetupQuadVertices();
	}

}
