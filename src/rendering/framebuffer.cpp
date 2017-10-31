#include "framebuffer.h"
#include "renderer.h"
#include "../ui/uigraphicscontext.h"

namespace MCR
{
	static const VmaAllocationCreateInfo gpuAllocationCI = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
	
	Framebuffer::Attachment Framebuffer::CreateColorAttachment(VkFormat format, uint32_t width, uint32_t height,
	                                                           VkImageUsageFlags extraUsageFlags)
	{
		Attachment attachment;
		
		//Creates the image
		VkImageCreateInfo imageCreateInfo;
		InitImageCreateInfo(imageCreateInfo, VK_IMAGE_TYPE_2D, format, width, height);
		imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | extraUsageFlags;
		CheckResult(vmaCreateImage(vulkan.allocator, &imageCreateInfo, &gpuAllocationCI,
		                           attachment.m_image.GetCreateAddress(),
		                           attachment.m_allocation.GetCreateAddress(), nullptr));
		
		//Creates the image view
		VkImageViewCreateInfo viewCreateInfo;
		InitImageViewCreateInfo(viewCreateInfo, *attachment.m_image, VK_IMAGE_VIEW_TYPE_2D,
		                        format, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		CheckResult(vkCreateImageView(vulkan.device, &viewCreateInfo, nullptr,
		                              attachment.m_imageView.GetCreateAddress()));
		
		return attachment;
	}
	
	void Framebuffer::Create(const RenderPasses& renderPasses, gsl::span<const VkImage> outputImages,
	                         uint32_t width, uint32_t height)
	{
		m_rendererFramebuffer.Reset();
		
		// ** Creates the color attachment image **
		m_colorAttachment = CreateColorAttachment(Renderer::ColorAttachmentFormat, width, height,
		                                          VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		
		// ** Creates the depth attachment image **
		VkImageCreateInfo depthImageCreateInfo;
		InitImageCreateInfo(depthImageCreateInfo, VK_IMAGE_TYPE_2D, vulkan.depthFormat, width, height);
		depthImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		
		CheckResult(vmaCreateImage(vulkan.allocator, &depthImageCreateInfo, &gpuAllocationCI,
		                           m_depthAttachment.m_image.GetCreateAddress(),
		                           m_depthAttachment.m_allocation.GetCreateAddress(), nullptr));
		
		// ** Creates the depth attachment image view **
		VkImageViewCreateInfo depthImageViewCreateInfo;
		InitImageViewCreateInfo(depthImageViewCreateInfo, *m_depthAttachment.m_image, VK_IMAGE_VIEW_TYPE_2D,
		                        vulkan.depthFormat, { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 });
		CheckResult(vkCreateImageView(vulkan.device, &depthImageViewCreateInfo, nullptr,
		                              m_depthAttachment.m_imageView.GetCreateAddress()));
		
		// ** Creates the god rays attachments **
		uint32_t godRaysWidth = width / PostProcessor::GodRaysDownscale;
		uint32_t godRaysHeight = height / PostProcessor::GodRaysDownscale;
		m_unblurredGodRaysAttachment = CreateColorAttachment(PostProcessor::GodRaysFormat, godRaysWidth, godRaysHeight);
		m_blurredGodRaysAttachment = CreateColorAttachment(PostProcessor::GodRaysFormat, width, height);
		
		m_outputs.resize(outputImages.size());
		for (int i = 0; i < outputImages.size(); i++)
		{
			// ** Creates the output image view **
			VkImageView outputImageView;
			VkImageViewCreateInfo outputImageViewCreateInfo;
			InitImageViewCreateInfo(outputImageViewCreateInfo, outputImages[i], VK_IMAGE_VIEW_TYPE_2D,
			                        SwapChain::GetImageFormat(), { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
			CheckResult(vkCreateImageView(vulkan.device, &outputImageViewCreateInfo, nullptr, &outputImageView));
			
			m_outputs[i].m_imageView = outputImageView;
			
			m_outputs[i].m_framebuffer = CreateFramebuffer(renderPasses.m_ui, SingleElementSpan(outputImageView),
			                                               width, height);
		}
		
		std::array<VkImageView, 2> rendererAttachments{ *m_colorAttachment.m_imageView, *m_depthAttachment.m_imageView };
		m_rendererFramebuffer = CreateFramebuffer(renderPasses.m_renderer, rendererAttachments, width, height);
		
		m_godRaysGenFramebuffer =
		        CreateFramebuffer(renderPasses.m_godRays, SingleElementSpan(*m_unblurredGodRaysAttachment.m_imageView),
		                          godRaysWidth, godRaysHeight);
		
		m_godRaysBlurFramebuffer = 
		        CreateFramebuffer(renderPasses.m_godRays, SingleElementSpan(*m_blurredGodRaysAttachment.m_imageView),
		                          width, height);
		
		m_width = width;
		m_height = height;
	}
}
