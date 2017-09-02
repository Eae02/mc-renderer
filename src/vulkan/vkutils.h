#pragma once

#include <vector>
#include <algorithm>
#include <glm/glm.hpp>

#include "vkhandle.h"
#include "instance.h"

namespace MCR
{
	constexpr size_t MaxQueuedFrames = 3;
	
	extern uint64_t frameIndex;
	extern uint64_t frameQueueIndex;
	
	inline void IncrementFrameIndex()
	{
		frameIndex++;
		frameQueueIndex = frameIndex % MaxQueuedFrames;
	}
	
	bool CanUseFormat(VkFormat format, VkFormatFeatureFlags requiredFeatures, VkImageTiling tiling);
	
	void CheckResult(VkResult result);
	
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
}
