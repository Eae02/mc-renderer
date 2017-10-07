#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <gsl/span>

namespace MCR
{
namespace SwapChain
{
	struct PresentImage
	{
		VkImage m_image;
		uint32_t m_width;
		uint32_t m_height;
	};
	
	void Initialize();
	
	void Create(bool enableVSync, bool force = false);
	
	uint32_t AquireImage(VkSemaphore& aquireSemaphoreOut);
	
	void Present(VkSemaphore waitSemaphore, uint32_t imageIndex);
	
	uint32_t GetImageCount();
	VkFormat GetImageFormat();
	
	gsl::span<const VkImage> GetImages();
	
	void Destroy();
}
}
