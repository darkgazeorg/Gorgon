// Native surface creation is separated from Vulkan.cpp as platform headers define macros like
// None and Success which conflict with Gorgon.
#ifdef LINUX
#	define VK_USE_PLATFORM_XLIB_KHR
#endif

#include <vulkan/vulkan.h>

#include <stdexcept>

namespace Gorgon :: GL {

	const char *GetNativeSurfaceExtension() {
		return VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
	}

	VkSurfaceKHR CreateNativeSurface(VkInstance instance, void *display, unsigned long window) {
		VkXlibSurfaceCreateInfoKHR createinfo{};
		createinfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
		createinfo.dpy = static_cast<Display *>(display);
		createinfo.window = window;

		VkSurfaceKHR surface;
		if(vkCreateXlibSurfaceKHR(instance, &createinfo, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("Cannot create window surface");
		}

		return surface;
	}

}
