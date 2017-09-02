#include "rendererframebuffer.h"
#include "renderer.h"

namespace MCR
{
	
	void RendererFramebuffer::Create(const Renderer& renderer, uint32_t width, uint32_t height)
	{
		VmaMemoryRequirements memoryRequirements = { };
		memoryRequirements.flags = VMA_MEMORY_REQUIREMENT_OWN_MEMORY_BIT;
		memoryRequirements.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		
		VmaAllocation imageAllocation;
		
		// ** Creates the color attachment image **
		VkImageCreateInfo colorImageCreateInfo;
		InitImageCreateInfo(colorImageCreateInfo, VK_IMAGE_TYPE_2D, Renderer::ColorAttachmentFormat, width, height);
		colorImageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
		                             VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		
		CheckResult(vmaCreateImage(vulkan.allocator, &colorImageCreateInfo, &memoryRequirements,
		                           m_colorAttachment.m_image.GetCreateAddress(), &imageAllocation, nullptr));
		
		m_colorAttachment.m_allocation.reset(imageAllocation);
		
		// ** Creates the color attachment image view **
		VkImageViewCreateInfo colorImageViewCreateInfo;
		InitImageViewCreateInfo(colorImageViewCreateInfo, *m_colorAttachment.m_image, VK_IMAGE_VIEW_TYPE_2D,
		                        Renderer::ColorAttachmentFormat, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		CheckResult(vkCreateImageView(vulkan.device, &colorImageViewCreateInfo, nullptr,
		                              m_colorAttachment.m_imageView.GetCreateAddress()));
		
		// ** Creates the depth attachment image **
		VkImageCreateInfo depthImageCreateInfo;
		InitImageCreateInfo(depthImageCreateInfo, VK_IMAGE_TYPE_2D, vulkan.depthFormat, width, height);
		depthImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		
		CheckResult(vmaCreateImage(vulkan.allocator, &depthImageCreateInfo, &memoryRequirements,
		                           m_depthAttachment.m_image.GetCreateAddress(), &imageAllocation, nullptr));
		
		m_depthAttachment.m_allocation.reset(imageAllocation);
		
		// ** Creates the depth attachment image view **
		VkImageViewCreateInfo depthImageViewCreateInfo;
		InitImageViewCreateInfo(depthImageViewCreateInfo, *m_depthAttachment.m_image, VK_IMAGE_VIEW_TYPE_2D,
		                        vulkan.depthFormat, { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 });
		CheckResult(vkCreateImageView(vulkan.device, &depthImageViewCreateInfo, nullptr,
		                              m_depthAttachment.m_imageView.GetCreateAddress()));
		
		std::array<VkImageView, 2> attachments = { *m_colorAttachment.m_imageView, *m_depthAttachment.m_imageView };
		m_framebuffer = CreateFramebuffer(renderer.GetRenderPass(), attachments, width, height);
		
		m_width = width;
		m_height = height;
	}
	
	void RendererFramebuffer::GetPresentImage(SwapChain::PresentImage& presentImageOut) const
	{
		presentImageOut.m_finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		presentImageOut.m_width = m_width;
		presentImageOut.m_height = m_height;
		presentImageOut.m_image = *m_colorAttachment.m_image;
	}
}