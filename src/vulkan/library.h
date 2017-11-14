#pragma once

#include <vulkan/vulkan.h>

inline namespace VKFunctions
{
	extern PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
}

namespace MCR
{
	bool LoadVulkanLibrary();
}
