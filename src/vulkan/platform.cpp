#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#ifdef SDL_VIDEO_DRIVER_X11
#define VK_USE_PLATFORM_XLIB_KHR
#define VK_USE_PLATFORM_XCB_KHR
#include <X11/Xlib-xcb.h>
#endif

#ifdef SDL_VIDEO_DRIVER_WAYLAND
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif

#ifdef SDL_VIDEO_DRIVER_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>
#include <stdexcept>

#ifdef __linux__
#include <dlfcn.h>
#endif

#include "instance.h"
#include "instanceextensionslist.h"

namespace MCR
{
	const char* GetPlatformSurfaceName(SDL_Window* window, const InstanceExtensionsList& instanceExtensionsList)
	{
		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		SDL_GetWindowWMInfo(window, &wmInfo);
		
		switch (wmInfo.subsystem)
		{
#ifdef SDL_VIDEO_DRIVER_X11
		case SDL_SYSWM_X11:
			if (instanceExtensionsList.IsExtensionSupported(VK_KHR_XLIB_SURFACE_EXTENSION_NAME))
				return VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
			if (instanceExtensionsList.IsExtensionSupported(VK_KHR_XCB_SURFACE_EXTENSION_NAME))
				return VK_KHR_XCB_SURFACE_EXTENSION_NAME;
			return nullptr;
#endif

#ifdef SDL_VIDEO_DRIVER_WAYLAND
			case SDL_SYSWM_WAYLAND:
			return VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
#endif

#ifdef SDL_VIDEO_DRIVER_WINDOWS
			case SDL_SYSWM_WINDOWS:
			return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
#endif
		default:
			return nullptr;
		}
	}
	
	VkSurfaceKHR CreateVulkanSurface(SDL_Window* window, const char* platformSurfaceName)
	{
		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		SDL_GetWindowWMInfo(window, &wmInfo);
		
		VkSurfaceKHR surface;
		
		switch (wmInfo.subsystem)
		{
#ifdef SDL_VIDEO_DRIVER_WINDOWS
		case SDL_SYSWM_WINDOWS:
		{
			auto vkCreateWin32SurfaceKHR = VK_GET_INSTANCE_FUNCTION(vkCreateWin32SurfaceKHR);
			
			VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
			createInfo.hinstance = GetModuleHandle(nullptr);
			createInfo.hwnd = wmInfo.info.win.window;
			
			vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);
			break;
		}
#endif
#ifdef SDL_VIDEO_DRIVER_X11
		case SDL_SYSWM_X11:
		{
			if (strcmp(platformSurfaceName, VK_KHR_XCB_SURFACE_EXTENSION_NAME) == 0)
			{
				auto vkCreateXcbSurfaceKHR = VK_GET_INSTANCE_FUNCTION(vkCreateXcbSurfaceKHR);
				
				VkXcbSurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR };
				createInfo.connection = XGetXCBConnection(wmInfo.info.x11.display);
				createInfo.window = static_cast<xcb_window_t>(wmInfo.info.x11.window);
				
				vkCreateXcbSurfaceKHR(vulkan.instance, &createInfo, nullptr, &surface);
			}
			else
			{
				auto vkCreateXlibSurfaceKHR = VK_GET_INSTANCE_FUNCTION(vkCreateXlibSurfaceKHR);
				
				VkXlibSurfaceCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR};
				createInfo.dpy = wmInfo.info.x11.display;
				createInfo.window = wmInfo.info.x11.window;
				
				vkCreateXlibSurfaceKHR(vulkan.instance, &createInfo, nullptr, &surface);
			}
			break;
		}
#endif
#ifdef SDL_VIDEO_DRIVER_WAYLAND
		case SDL_SYSWM_WAYLAND:
		{
			auto vkCreateWaylandSurfaceKHR = VK_GET_INSTANCE_FUNCTION(vkCreateWaylandSurfaceKHR);
			
			VkWaylandSurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR };
			createInfo.display = wmInfo.info.wl.display;
			createInfo.surface = wmInfo.info.wl.surface;
			
			vkCreateWaylandSurfaceKHR(instance, &createInfo, nullptr, &surface);
			break;
		}
#endif
		default:
			throw std::runtime_error("Unsupported window system.");
		}
		
		return surface;
	}
}
