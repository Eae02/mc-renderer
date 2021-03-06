#pragma once

#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "queue.h"

namespace MCR
{
	enum
	{
		QUEUE_FAMILY_GRAPHICS, //Must also support compute, transfer and presentation
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
		VkFormat depthStencilFormat;
		
		struct
		{
			uint64_t uniformBufferOffsetAlignment;
			uint64_t storageBufferOffsetAlignment;
			float timestampMillisecondPeriod;
			bool hasMultiDrawIndirect;
			bool hasTessellation;
			uint32_t maxComputeWorkGroupInvocations;
			uint32_t maxComputeWorkGroupSize[3];
		} limits;
	};
	
	extern VulkanInstance vulkan;
	
	void InitializeVulkan(SDL_Window* window);
	
	void DestroyVulkan();
}
