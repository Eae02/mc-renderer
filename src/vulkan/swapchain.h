#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>

namespace MCR
{
namespace SwapChain
{
	struct PresentImage
	{
		VkImage m_image;
		uint32_t m_width;
		uint32_t m_height;
		VkImageLayout m_finalLayout;
	};
	
	void Initialize();
	
	void Create(bool enableVSync, bool force = false);
	
	void Present(VkSemaphore waitSemaphore, VkPipelineStageFlags waitStage);
	
	void SetPresentImage(const PresentImage& presentImage);
	
	void Destroy();
}
}
