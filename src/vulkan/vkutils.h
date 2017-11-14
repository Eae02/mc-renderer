#pragma once

#include <vector>
#include <algorithm>
#include <glm/glm.hpp>
#ifdef _MSC_VER
#include <intrin.h>
#endif

#include "swapchain.h"
#include "vkhandle.h"
#include "instance.h"

namespace MCR
{
	extern uint64_t frameIndex;
	extern uint64_t frameQueueIndex;
	
	extern const VkComponentMapping IdentityComponentMapping;
	
	inline void IncrementFrameIndex()
	{
		frameIndex++;
	}
	
	bool CanUseFormat(VkFormat format, VkFormatFeatureFlags requiredFeatures, VkImageTiling tiling);
	
	void CheckResult(VkResult result);
	
	inline uint32_t CalculateMipLevels(uint32_t minResolution)
	{
#if defined(__GNUC__)
		return 31 - __builtin_clz(minResolution);
#elif defined(_MSC_VER)
		unsigned long trailingZero;
		_BitScanReverse(&trailingZero, minResolution);
		return trailingZero;
#else
#
		"Unknown compiler"
#endif
	}
	
	inline void WaitForFence(VkFence fence, uint64_t timeout = UINT64_MAX)
	{
		CheckResult(vkWaitForFences(vulkan.device, 1, &fence, VK_TRUE, timeout));
	}
	
	inline void WaitForFences(gsl::span<const VkFence> fences, bool waitForAll = true, uint64_t timeout = UINT64_MAX)
	{
		CheckResult(vkWaitForFences(vulkan.device, fences.size(), fences.data(), waitForAll, timeout));
	}
	
	inline void InitImageCreateInfo(VkImageCreateInfo& imageCreateInfo, VkImageType imageType, VkFormat format,
	                                uint32_t width, uint32_t height, uint32_t depth = 1)
	{
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.pNext = nullptr;
		imageCreateInfo.flags = 0;
		imageCreateInfo.imageType = imageType;
		imageCreateInfo.format = format;
		imageCreateInfo.extent = { width, height, depth };
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage = 0;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.queueFamilyIndexCount = 0;
		imageCreateInfo.pQueueFamilyIndices = nullptr;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	}
	
	inline void InitImageViewCreateInfo(VkImageViewCreateInfo& imageViewCreateInfo, VkImage image, VkImageViewType type,
	                                    VkFormat format, const VkImageSubresourceRange& subresourceRange)
	{
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.pNext = nullptr;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.image = image;
		imageViewCreateInfo.viewType = type;
		imageViewCreateInfo.format = format;
		imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
		                                   VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
		imageViewCreateInfo.subresourceRange = subresourceRange;
	}
	
	inline void InitImageMemoryBarrier(VkImageMemoryBarrier& barrier, VkImage image)
	{
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.pNext = nullptr;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		barrier.image = image;
	}
	
	inline void InitBufferCreateInfo(VkBufferCreateInfo& bufferCreateInfo, VkBufferUsageFlags usage, uint64_t size)
	{
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.pNext = nullptr;
		bufferCreateInfo.flags = 0;
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = usage;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferCreateInfo.queueFamilyIndexCount = 0;
		bufferCreateInfo.pQueueFamilyIndices = nullptr;
	}
	
	inline void InitShaderStageCreateInfo(VkPipelineShaderStageCreateInfo& createInfo, VkShaderStageFlagBits stage,
	                                      VkShaderModule module)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.stage = stage;
		createInfo.module = module;
		createInfo.pName = "main";
		createInfo.pSpecializationInfo = nullptr;
	}
	
	inline VkHandle<VkQueryPool> CreateQueryPool(VkQueryType type, uint32_t count)
	{
		const VkQueryPoolCreateInfo createInfo = 
		{
			/* sType              */ VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
			/* pNext              */ nullptr,
			/* flags              */ 0,
			/* queryType          */ type,
			/* queryCount         */ count,
			/* pipelineStatistics */ 0
		};
		
		VkQueryPool queryPool;
		CheckResult(vkCreateQueryPool(vulkan.device, &createInfo, nullptr, &queryPool));
		return queryPool;
	}
	
	inline VkHandle<VkFramebuffer> CreateFramebuffer(VkRenderPass renderPass, gsl::span<const VkImageView> attachments,
	                                                 uint32_t width, uint32_t height, uint32_t layers = 1)
	{
		const VkFramebufferCreateInfo createInfo = 
		{
			/* sType           */ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			/* pNext           */ nullptr,
			/* flags           */ 0,
			/* renderPass      */ renderPass,
			/* attachmentCount */ gsl::narrow<uint32_t>(attachments.size()),
			/* pAttachments    */ attachments.data(),
			/* width           */ width,
			/* height          */ height,
			/* layers          */ layers
		};
		
		VkFramebuffer framebuffer;
		CheckResult(vkCreateFramebuffer(vulkan.device, &createInfo, nullptr, &framebuffer));
		return framebuffer;
	}
	
	inline void SetColorClearValue(VkClearValue& clearValue, const glm::vec4& color)
	{
		for (int i = 0; i < 4; i++)
		{
			clearValue.color.float32[i] = color[i];
		}
	}
	
	inline void SetDepthStencilClearValue(VkClearValue& clearValue, float depth, uint32_t stencil)
	{
		clearValue.depthStencil.depth = depth;
		clearValue.depthStencil.stencil = stencil;
	}
	
	inline VkHandle<VkCommandPool> CreateCommandPool(int queue, VkCommandPoolCreateFlags flags)
	{
		VkCommandPoolCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = flags;
		createInfo.queueFamilyIndex = vulkan.queueFamilies[queue];
		
		VkCommandPool commandPool;
		CheckResult(vkCreateCommandPool(vulkan.device, &createInfo, nullptr, &commandPool));
		return commandPool;
	}
	
	inline VkHandle<VkFence> CreateVkFence() noexcept
	{
		VkFence fence;
		
		VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		CheckResult(vkCreateFence(vulkan.device, &createInfo, nullptr, &fence));
		
		return fence;
	}
	
	inline VkHandle<VkSemaphore> CreateVkSemaphore() noexcept
	{
		VkSemaphore semaphore;
		
		VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		CheckResult(vkCreateSemaphore(vulkan.device, &createInfo, nullptr, &semaphore));
		
		return semaphore;
	}
	
	void CreateStagingBuffer(uint64_t size, VkBuffer* buffer, VmaAllocation* allocation, void** memory);
}
