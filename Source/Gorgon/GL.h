#pragma once

#include <stdint.h>

#include "Geometry/Point.h"
#include "Geometry/Size.h"
#include "Geometry/Bounds.h"
#include "Geometry/Rectangle.h"
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

#if defined(GORGON_GL_OPENGL) || defined(GORGON_GL_VULKAN)
		/// This is a GLTexture descriptor. This may change depending on the GL used. It will always be
		/// copy constructible/assignable and comparable.
		typedef uint32_t Texture;
#endif

		/// Initializes the underlying graphics library. Should be called after a context is activated.
		void Initialize();

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

		/// Selects the texture that the active shader will use from the given texture unit
		void ActivateTexture(Texture texture, int unit);

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

		/// Sets the color that will be used by Clear.
		void SetClearColor(float r, float g, float b, float a);

		/// Sets default blending parameters as current.
		void SetDefaultBlending();

		/// Sets blending parameters used to draw to a mask as current. Mask will be multiplied by the
		/// color of the source.
		void SetMaskBlending();

		/// Returns whether drawing is limited to the clip rectangle.
		bool IsClipEnabled();

		/// Sets whether drawing is limited to the clip rectangle.
		void SetClipEnabled(bool enabled);

		/// Sets the clip rectangle. Its origin is at the bottom left of the target, regardless of the GL used.
		void SetClipRectangle(const Geometry::Rectangle &rectangle);

		/// Prepares quad vertices for the current context. Should be called after a context is activated.
		void SetupQuadVertices();

		/// Prepares to draw the quad vertices.
		void ActivateQuadVertices();

		/// Draws a quad using the active shader.
		void DrawQuadVertices();

		/// The logger that is used for GL operations. Default is unset and will not log anything. Use Initialize... to begin logging.
        extern Gorgon::Utils::Logger log;
	}
}
