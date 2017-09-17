#include "swapchain.h"
#include "instance.h"
#include "vkutils.h"
#include "commandbuffer.h"
#include "../utils.h"

#include <vector>
#include <algorithm>

namespace MCR
{
namespace SwapChain
{
	static VkHandle<VkSwapchainKHR> swapChain;
	
	static std::vector<VkImage> swapChainImages;
	
	static std::vector<CommandBuffer> blitCommandBuffers;
	
	static VkSurfaceFormatKHR surfaceFormat;
	
	static PresentImage presentImage;
	
	static VkExtent2D dimensions;
	static bool enableVSync;
	
	static VkHandle<VkSemaphore> aquireSemaphore;
	static VkHandle<VkSemaphore> blitSemaphore;
	
	inline VkSurfaceFormatKHR SelectSurfaceFormat()
	{
		//Gets the supported surface formats.
		uint32_t numFormats;
		vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan.physicalDevice, vulkan.surface, &numFormats, nullptr);
		std::vector<VkSurfaceFormatKHR> surfaceFormats(numFormats);
		vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan.physicalDevice, vulkan.surface, &numFormats, surfaceFormats.data());
		
		const VkFormat supportedFormats[] =
		{
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_FORMAT_B8G8R8A8_UNORM,
			VK_FORMAT_R8G8B8_UNORM,
			VK_FORMAT_B8G8R8_UNORM
		};
		
		if (surfaceFormats.size() == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
		{
			//There are no preferred surface formats, so we will use VK_FORMAT_R8G8B8A8_SRGB.
			VkSurfaceFormatKHR format;
			format.format = supportedFormats[0];
			format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			return format;
		}
		
		//Searches for a supported format.
		for (const VkSurfaceFormatKHR& format : surfaceFormats)
		{
			if (std::find(MAKE_RANGE(supportedFormats), format.format) != std::end(supportedFormats))
				return format;
		}
		
		throw std::runtime_error("No supported surface format found.");
	}
	
	inline VkPresentModeKHR SelectPresentMode(bool t_enableVSync)
	{
		// ** Enumerates supported present modes **
		uint32_t numPresentModes;
		vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan.physicalDevice, vulkan.surface, &numPresentModes, nullptr);
		std::vector<VkPresentModeKHR> presentModes(numPresentModes);
		vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan.physicalDevice, vulkan.surface,
		                                          &numPresentModes, presentModes.data());
		
		auto CanUsePresentMode = [&] (VkPresentModeKHR presentMode)
		{
			return std::find(MAKE_RANGE(presentModes), presentMode) != presentModes.end();
		};
		
		if (!t_enableVSync)
		{
			//Try to use immediate present mode if vsync is disabled.
			if (CanUsePresentMode(VK_PRESENT_MODE_IMMEDIATE_KHR))
			{
				return VK_PRESENT_MODE_IMMEDIATE_KHR;
			}
			
			Log("Disabling V-Sync is not supported by this driver (it does not support immediate present mode).");
		}
		
		//Use mailbox if available, otherwise FIFO.
		if (CanUsePresentMode(VK_PRESENT_MODE_MAILBOX_KHR))
		{
			return VK_PRESENT_MODE_MAILBOX_KHR;
		}
		
		return VK_PRESENT_MODE_FIFO_KHR;
	}
	
	inline bool CheckSurfaceCapabilities(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		return bool(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	}
	
	void Initialize()
	{
		surfaceFormat = SelectSurfaceFormat();
		
		aquireSemaphore = CreateVkSemaphore();
		blitSemaphore = CreateVkSemaphore();
	}
	
	static void RecordBlitCommandBuffers()
	{
		while (swapChainImages.size() > blitCommandBuffers.size())
		{
			blitCommandBuffers.emplace_back(vulkan.stdCommandPools[QUEUE_FAMILY_GRAPHICS]);
		}
		
		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			CommandBuffer& cb = blitCommandBuffers[i];
			
			cb.Begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
			
			VkImageMemoryBarrier barrier;
			
			//Barrier which sets the layout of the swap chain image to TRANSFER_DST_OPTIMAL.
			InitImageMemoryBarrier(barrier, swapChainImages[i]);
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			
			cb.PipelineBarrier(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			                   { }, { }, SingleElementSpan(barrier));
			
			const VkImageBlit blitRegion = 
			{
				/* srcSubresource */ { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
				/* srcOffsets[2]  */ { { 0, 0, 0 }, { presentImage.m_width, presentImage.m_height, 1 } },
				/* dstSubresource */ { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
				/* dstOffsets[2]  */ { { 0, 0, 0 }, { dimensions.width, dimensions.height, 1 } }
			};
			cb.BlitImage(presentImage.m_image, VK_IMAGE_LAYOUT_GENERAL, swapChainImages[i],
			             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, blitRegion, VK_FILTER_NEAREST);
			
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = 0;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			
			cb.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
			                   { }, { }, SingleElementSpan(barrier));
			
			cb.End();
		}
	}
	
	inline const char* GetPresentModeName(VkPresentModeKHR presentMode)
	{
		switch (presentMode)
		{
		case VK_PRESENT_MODE_IMMEDIATE_KHR:
			return "immediate";
		case VK_PRESENT_MODE_MAILBOX_KHR:
			return "mailbox";
		case VK_PRESENT_MODE_FIFO_KHR:
			return "FIFO";
		default:
			return "unknown";
		}
	}
	
	void Create(bool t_enableVSync, bool force)
	{
		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan.physicalDevice, vulkan.surface, &capabilities);
		
		if (!CheckSurfaceCapabilities(capabilities))
		{
			throw std::runtime_error("The vulkan device does not support all required surface usages.");
		}
		
		if (!force && dimensions.width == capabilities.currentExtent.width &&
		    dimensions.height == capabilities.currentExtent.height && enableVSync == t_enableVSync)
		{
			return;
		}
		
		uint32_t imageCount = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount != 0 && imageCount > capabilities.maxImageCount)
		{
			imageCount = capabilities.maxImageCount;
		}
		
		dimensions = capabilities.currentExtent;
		enableVSync = t_enableVSync;
		
		VkSwapchainCreateInfoKHR chainCreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		
		chainCreateInfo.surface = vulkan.surface;
		chainCreateInfo.minImageCount = imageCount;
		chainCreateInfo.imageFormat = surfaceFormat.format;
		chainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
		chainCreateInfo.imageExtent = capabilities.currentExtent;
		chainCreateInfo.imageArrayLayers = 1;
		chainCreateInfo.preTransform = capabilities.currentTransform;
		chainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		chainCreateInfo.presentMode = SelectPresentMode(t_enableVSync);
		chainCreateInfo.clipped = true;
		chainCreateInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		
		Log("Creating swap chain, present mode: ", GetPresentModeName(chainCreateInfo.presentMode), ", images: ", imageCount);
		
		CheckResult(vkCreateSwapchainKHR(vulkan.device, &chainCreateInfo, nullptr, swapChain.GetCreateAddress()));
		
		vkGetSwapchainImagesKHR(vulkan.device, *swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(vulkan.device, *swapChain, &imageCount, swapChainImages.data());
		
		if (presentImage.m_image)
		{
			RecordBlitCommandBuffers();
		}
	}
	
	void Destroy()
	{
		swapChain.Reset();
		
		aquireSemaphore.Reset();
		blitSemaphore.Reset();
		
		blitCommandBuffers.clear();
	}
	
	void SetPresentImage(const PresentImage& t_presentImage)
	{
		presentImage = t_presentImage;
		
		if (!swapChainImages.empty())
		{
			RecordBlitCommandBuffers();
		}
	}
	
	uint32_t AquireImage()
	{
		uint32_t imageIndex;
		VkResult aquireResult = vkAcquireNextImageKHR(vulkan.device, *swapChain, UINT64_MAX,
		                                              *aquireSemaphore, nullptr, &imageIndex);
		
		switch (aquireResult)
		{
		case VK_SUCCESS:
			return imageIndex;
			
		case VK_SUBOPTIMAL_KHR:
		case VK_ERROR_OUT_OF_DATE_KHR:
			Create(enableVSync, true);
			return AquireImage();
			
		default:
			throw std::runtime_error("Error acquiring next image from swap chain.");
		}
	}
	
	void Present(VkSemaphore waitSemaphore, VkPipelineStageFlags waitDstStageMask)
	{
		uint32_t imageIndex = AquireImage();
		
		VkSemaphore waitSemaphores[2] = { waitSemaphore, *aquireSemaphore };
		VkPipelineStageFlags waitStageMasks[2] = { waitDstStageMask, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
		
		VkSubmitInfo blitSubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		blitSubmitInfo.commandBufferCount = 1;
		blitSubmitInfo.pCommandBuffers = &blitCommandBuffers[imageIndex].GetVkCB();
		blitSubmitInfo.waitSemaphoreCount = 2;
		blitSubmitInfo.pWaitSemaphores = waitSemaphores;
		blitSubmitInfo.pWaitDstStageMask = waitStageMasks;
		blitSubmitInfo.signalSemaphoreCount = 1;
		blitSubmitInfo.pSignalSemaphores = &*blitSemaphore;
		
		vulkan.queues[QUEUE_FAMILY_GRAPHICS]->Submit(1, &blitSubmitInfo, nullptr);
		
		VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &*blitSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &*swapChain;
		presentInfo.pImageIndices = &imageIndex;
		
		switch (vulkan.queues[QUEUE_FAMILY_GRAPHICS]->Present(presentInfo))
		{
		case VK_SUCCESS:
			break;
			
		case VK_ERROR_OUT_OF_DATE_KHR:
		case VK_SUBOPTIMAL_KHR:
			Create(enableVSync, true);
			break;
			
		default:
			throw std::runtime_error("Error during image presentation.");
		}
	}
}
}
