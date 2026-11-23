// Native surface creation is separated from Vulkan.cpp as platform headers define macros like
// None, Success, CreateWindow, Rectangle, min, and max which conflict with Gorgon.
#ifdef LINUX
#	define VK_USE_PLATFORM_XLIB_KHR
#elif defined(WIN32)
#	define WIN32_LEAN_AND_MEAN
#	define NOMINMAX
#	define VK_USE_PLATFORM_WIN32_KHR
#	include <windows.h>
#endif

#include <vulkan/vulkan.h>

#include <cstdint>
#include <stdexcept>

namespace Gorgon :: GL {

	const char *GetNativeSurfaceExtension() {
#ifdef LINUX
		return VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
#elif defined(WIN32)
		return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
#else
#		error "Unsupported platform for Vulkan surface"
#endif
	}

	VkSurfaceKHR CreateNativeSurface(VkInstance instance, void *display, uintptr_t window) {
#ifdef LINUX
		VkXlibSurfaceCreateInfoKHR createinfo{};
		createinfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
		createinfo.dpy = static_cast<Display *>(display);
		createinfo.window = static_cast<::Window>(window);

		VkSurfaceKHR surface;
		if(vkCreateXlibSurfaceKHR(instance, &createinfo, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("Cannot create window surface");
		}

		return surface;
#elif defined(WIN32)
		VkWin32SurfaceCreateInfoKHR createinfo{};
		createinfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createinfo.hinstance = static_cast<HINSTANCE>(display);
		createinfo.hwnd = reinterpret_cast<HWND>(window);

		VkSurfaceKHR surface;
		if(vkCreateWin32SurfaceKHR(instance, &createinfo, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("Cannot create window surface");
		}

		return surface;
#endif
	}

}
