#include "func.h"

#include <stdexcept>

inline namespace VKFunctions
{
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
	
#define VK_GLOBAL_LEVEL_FUNCTION(fun) PFN_##fun fun;
#define VK_INSTANCE_LEVEL_FUNCTION(fun) PFN_##fun fun;
#define VK_DEVICE_LEVEL_FUNCTION(fun) PFN_##fun fun;
	
#include "functionslist.inl"
	
#undef VK_GLOBAL_LEVEL_FUNCTION
#undef VK_INSTANCE_LEVEL_FUNCTION
#undef VK_DEVICE_LEVEL_FUNCTION
}

namespace MCR
{
	void LoadGlobalVulkanFunctions()
	{
#define VK_GLOBAL_LEVEL_FUNCTION(fun) \
if(!(fun = reinterpret_cast<PFN_##fun>(vkGetInstanceProcAddr(nullptr, #fun)))) \
{ throw std::runtime_error("Could not load global level function: " #fun "!"); }
		
#include "functionslist.inl"
		
#undef VK_GLOBAL_LEVEL_FUNCTION
	}
	
	void LoadInstanceVulkanFunctions(VkInstance instance)
	{
#define VK_INSTANCE_LEVEL_FUNCTION(fun) \
if(!(fun = reinterpret_cast<PFN_##fun>(vkGetInstanceProcAddr(instance, #fun)))) \
{ throw std::runtime_error("Could not load instance level function: " #fun "!"); }
		
#include "functionslist.inl"
		
#undef VK_INSTANCE_LEVEL_FUNCTION
	}
	
	void LoadDeviceVulkanFunctions(VkDevice device)
	{
#define VK_DEVICE_LEVEL_FUNCTION(fun) \
if(!(fun = reinterpret_cast<PFN_##fun>(vkGetDeviceProcAddr(device, #fun)))) \
{ throw std::runtime_error("Could not load instance level function: " #fun "!"); }
		
#include "functionslist.inl"
		
#undef VK_DEVICE_LEVEL_FUNCTION
	}
}
