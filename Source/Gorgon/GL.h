#pragma once

#include <stdint.h>

#include "Geometry/Point.h"
#include "Geometry/Size.h"
#include "Geometry/Bounds.h"
#include "Graphics/Color.h"
#include "Utils/Logging.h"
#include "Containers/Image.h"

namespace Gorgon {
	
	/// This namespace contains underlying graphics library functions. These functions are presented
	/// in a OS/Window manager generic way. However, direct access to GL functions might be necessary
	/// in many cases. In those cases you may include GL/OpenGL.h library header. This header might
	/// expose operating system dependent headers. In the future there might be more supported GL libraries.
	/// In that case, you should choose to include the library you wish to use.
	namespace GL {

		class FrameBuffer;

#ifdef GORGON_GL_OPENGL
		/// This is a GLTexture descriptor. This may change depending on the GL used. It will always be
		/// copy constructible/assignable and comparable.
		typedef uint32_t Texture;
#endif

		/// This function generates a texture from the given image data.
		Texture GenerateTexture(const Containers::Image &data);

		/// This function generates a texture from the given image data.
		Texture GenerateEmptyTexture(const Geometry::Size &size, Graphics::ColorMode mode);

		/// Resizes the given texture to the specified size. The data in the texture cannot 
		/// be trusted after this call.
		void ResizeTexture(Texture texture, const Geometry::Size &size, Graphics::ColorMode mode);

		/// Updates the given texture to contain the given data
		void UpdateTexture(Texture texture, const Containers::Image &data);

		/// Copies the data from the given image to the texture starting from the given target
		void CopyToTexture(Texture texture, const Containers::Image &data, Geometry::Point target);

		/// Copies the data from the given image to the texture starting from specified boundary of the given target
		void CopyToTexture(Texture texture, const Containers::Image &data, Geometry::Bounds source, Geometry::Point target);

		/// Destroys the given texture
		void DestroyTexture(Texture texture);

		/// Begins using the given frame buffer. 
		void RenderToTexture(FrameBuffer &buffer);

		/// Stops rendering to a texture and start rendering to a buffer
		void RenderToScreen();

		/// Performs first time initialization on GL context
		void SetupContext(const Geometry::Size &size);

		/// Resizes the active context
		void Resize(const Geometry::Size &size);

		/// Clears the window pointed by the active context
		void Clear();

		/// Sets default clear parameters as current.
		void SetDefaultClear();

		/// Sets default blending parameters as current.
		void SetDefaultBlending();

		/// The logger that is used for GL operations. Default is unset and will not log anything. Use Initialize... to begin logging.
        extern Gorgon::Utils::Logger log;
	}
}
