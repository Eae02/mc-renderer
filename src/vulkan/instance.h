#pragma once

#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "queue.h"

namespace MCR
{
	enum
	{
		QUEUE_FAMILY_GRAPHICS,
		QUEUE_FAMILY_COMPUTE,
		QUEUE_FAMILY_TRANSFER,
		QUEUE_FAMILY_COUNT
	};
	
	struct VulkanInstance
	{
		VkInstance instance;
		VkSurfaceKHR surface;
		uint32_t queueFamilies[QUEUE_FAMILY_COUNT];
		VkPhysicalDevice physicalDevice;
		VkDevice device;
		VmaAllocator allocator;
		Queue* queues[QUEUE_FAMILY_COUNT];
		VkCommandPool stdCommandPools[QUEUE_FAMILY_COUNT];
		VkFormat depthFormat;
		
		struct
		{
			uint64_t uniformBufferOffsetAlignment;
			uint64_t storageBufferOffsetAlignment;
			bool hasMultiDrawIndirect;
		} limits;
	};
	
	extern VulkanInstance vulkan;
	
	void InitializeVulkan(SDL_Window* window);
	
	void DestroyVulkan();
}
