#pragma once

#include <vulkan/vulkan.h>

#include "library.h"

inline namespace VKFunctions
{
#define VK_GLOBAL_LEVEL_FUNCTION(fun) extern PFN_##fun fun;
#define VK_INSTANCE_LEVEL_FUNCTION(fun) extern PFN_##fun fun;
#define VK_DEVICE_LEVEL_FUNCTION(fun) extern PFN_##fun fun;
	
#include "functionslist.inl"
	
#undef VK_GLOBAL_LEVEL_FUNCTION
#undef VK_INSTANCE_LEVEL_FUNCTION
#undef VK_DEVICE_LEVEL_FUNCTION
}

#define VK_GET_DEVICE_FUNCTION(name) reinterpret_cast<PFN_##name>(vkGetDeviceProcAddr(MCR::vulkan.device, #name))
#define VK_GET_INSTANCE_FUNCTION(name) reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(MCR::vulkan.instance, #name))
