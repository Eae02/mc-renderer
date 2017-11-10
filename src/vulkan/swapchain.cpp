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
	
	static uint32_t imageCount;
	
	static VkSurfaceFormatKHR surfaceFormat;
	
	static VkExtent2D dimensions;
	static bool enableVSync;
	
	static std::vector<VkHandle<VkSemaphore>> acquireSemaphores;
	static uint32_t acquireSemaphoreIndex = 0;
	
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
		return bool(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	}
	
	void Initialize()
	{
		surfaceFormat = SelectSurfaceFormat();
		
		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan.physicalDevice, vulkan.surface, &capabilities);
		
		imageCount = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount != 0 && imageCount > capabilities.maxImageCount)
		{
			imageCount = capabilities.maxImageCount;
		}
		
		swapChainImages.resize(imageCount);
		
		acquireSemaphores.resize(imageCount);
		for (uint32_t i = 0; i < imageCount; i++)
		{
			acquireSemaphores[i] = CreateVkSemaphore();
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
		chainCreateInfo.clipped = VK_TRUE;
		chainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		
		CheckResult(vkCreateSwapchainKHR(vulkan.device, &chainCreateInfo, nullptr, swapChain.GetCreateAddress()));
		
		uint32_t createdImageCount;
		vkGetSwapchainImagesKHR(vulkan.device, *swapChain, &createdImageCount, nullptr);
		
		if (createdImageCount != imageCount)
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error Creating Swapchain",
			                         "Number of swapchain images changed at runtime.", nullptr);
			std::exit(1);
		}
		
		vkGetSwapchainImagesKHR(vulkan.device, *swapChain, &createdImageCount, swapChainImages.data());
		
		Log("Creating swap chain, present mode: ", GetPresentModeName(chainCreateInfo.presentMode),
		    ", images: ", createdImageCount);
	}
	
	void Destroy()
	{
		swapChain.Reset();
		acquireSemaphores.clear();
	}
	
	uint32_t AcquireImage(VkSemaphore& acquireSemaphoreOut)
	{
		acquireSemaphoreOut = *acquireSemaphores[acquireSemaphoreIndex];
		acquireSemaphoreIndex = (acquireSemaphoreIndex + 1) % imageCount;
		
	preAcquire:
		uint32_t imageIndex;
		VkResult acquireResult = vkAcquireNextImageKHR(vulkan.device, *swapChain, UINT64_MAX,
		                                               acquireSemaphoreOut, nullptr, &imageIndex);
		
		switch (acquireResult)
		{
		case VK_SUCCESS:
			return imageIndex;
			
		case VK_SUBOPTIMAL_KHR:
		case VK_ERROR_OUT_OF_DATE_KHR:
			Create(enableVSync, true);
			goto preAcquire;
			
		default:
			throw std::runtime_error("Error acquiring next image from swap chain.");
		}
	}
	
	void Present(VkSemaphore waitSemaphore, uint32_t imageIndex)
	{
		VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &waitSemaphore;
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
	
	uint32_t GetImageCount()
	{
		return imageCount;
	}
	
	VkFormat GetImageFormat()
	{
		return surfaceFormat.format;
	}
	
	gsl::span<const VkImage> GetImages()
	{
		return swapChainImages;
	}
}
}
