#pragma once

#include "../GL.h"

namespace Gorgon :: GL {

	/// Rendering data of a window: surface, swapchain and frames in flight. There is a single device
	/// that is shared between windows, thus textures and shaders can be used with any window. This
	/// takes the place of a GL context.
	struct WindowContext;

	/// Creates a new context for the given native window. Vulkan is initialized with the first window
	/// as physical device selection depends on its surface. Throws if Vulkan cannot be used.
	WindowContext *CreateWindowContext(void *display, unsigned long window, const Geometry::Size &size);

	/// Destroys the given context. This should be called before its window is destroyed.
	void DestroyWindowContext(WindowContext *context);

	/// Selects the context that will be used for rendering. It can be nullptr to stop rendering.
	void MakeCurrent(WindowContext *context);

	/// Submits the commands that are recorded since the last Present and presents the result.
	void Present();

	/// Destroys all Vulkan objects, nothing can be rendered after this call. This is optional: Gorgon has no
	/// shutdown stage and these objects are reclaimed by the operating system when the program ends. It is 
	/// useful with validation layers, which report leaked objects while the device is being destroyed.
	void Shutdown();
}
