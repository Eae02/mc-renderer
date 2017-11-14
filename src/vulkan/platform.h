#pragma once

#include <vulkan/vulkan.h>
#include <SDL2/SDL_video.h>

namespace MCR
{
	const char* GetPlatformSurfaceName(SDL_Window* window, const class InstanceExtensionsList& instanceExtensionsList);
	
	VkSurfaceKHR CreateVulkanSurface(SDL_Window* window, const char* platformSurfaceName);
}
