#include "vkutils.h"
#include "instance.h"
#include "graphicsdevicelostexception.h"

#include <sstream>

namespace MCR
{
	const VkComponentMapping IdentityComponentMapping = 
	{
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY
	};
	
	uint64_t frameIndex = 0;
	uint64_t frameQueueIndex = 0;
	
	bool CanUseFormat(VkFormat format, VkFormatFeatureFlags requiredFeatures, VkImageTiling tiling)
	{
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(vulkan.physicalDevice, format, &properties);
		
		VkFormatFeatureFlags supportedFeatures;
		if (tiling == VK_IMAGE_TILING_LINEAR)
			supportedFeatures = properties.linearTilingFeatures;
		else if (tiling == VK_IMAGE_TILING_OPTIMAL)
			supportedFeatures = properties.optimalTilingFeatures;
		
		return supportedFeatures && (supportedFeatures & requiredFeatures) == requiredFeatures;
	}
	
	void CheckResult(VkResult result)
	{
		if (result != VK_SUCCESS)
		{
			if (result == VK_ERROR_OUT_OF_HOST_MEMORY)
				throw std::bad_alloc();
			if (result == VK_ERROR_DEVICE_LOST)
				throw GraphicsDeviceLostException();
			
			std::ostringstream errorMsg;
			
			errorMsg << "Error in vulkan call, returned: " << static_cast<int>(result);
			
#define RES_CASE(c) case c: errorMsg << " (" #c ")"; break
			
			switch (result)
			{
				RES_CASE(VK_ERROR_OUT_OF_DEVICE_MEMORY);
				RES_CASE(VK_ERROR_INITIALIZATION_FAILED);
				RES_CASE(VK_ERROR_DEVICE_LOST);
				RES_CASE(VK_ERROR_MEMORY_MAP_FAILED);
				RES_CASE(VK_ERROR_LAYER_NOT_PRESENT);
				RES_CASE(VK_ERROR_EXTENSION_NOT_PRESENT);
				RES_CASE(VK_ERROR_FEATURE_NOT_PRESENT);
				RES_CASE(VK_ERROR_INCOMPATIBLE_DRIVER);
				RES_CASE(VK_ERROR_TOO_MANY_OBJECTS);
				RES_CASE(VK_ERROR_FORMAT_NOT_SUPPORTED);
				default: break;
			}
			
			errorMsg << ".";
			
			throw std::runtime_error(errorMsg.str());
		}
	}
}
