#include "framebuffer.h"
#include "renderer.h"
#include "../ui/uigraphicscontext.h"
#include "fbformats.h"

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
	
	Framebuffer::Attachment Framebuffer::CreateDepthAttachment(uint32_t width, uint32_t height,
	                                                           VkImageUsageFlags extraUsageFlags)
	{
		Attachment attachment;
		
		//Creates the image
		VkImageCreateInfo imageCreateInfo;
		InitImageCreateInfo(imageCreateInfo, VK_IMAGE_TYPE_2D, vulkan.depthStencilFormat, width, height);
		imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | extraUsageFlags;
		
		CheckResult(vmaCreateImage(vulkan.allocator, &imageCreateInfo, &gpuAllocationCI,
		                           attachment.m_image.GetCreateAddress(),
		                           attachment.m_allocation.GetCreateAddress(), nullptr));
		
		//Creates the image view
		VkImageViewCreateInfo viewCreateInfo;
		InitImageViewCreateInfo(viewCreateInfo, *attachment.m_image, VK_IMAGE_VIEW_TYPE_2D,
		                        vulkan.depthStencilFormat, { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 });
		CheckResult(vkCreateImageView(vulkan.device, &viewCreateInfo, nullptr,
		                              attachment.m_imageView.GetCreateAddress()));
		
		return attachment;
	}
	
	void Framebuffer::Create(const RenderPasses& renderPasses, gsl::span<const VkImage> outputImages,
	                         uint32_t width, uint32_t height)
	{
		m_rendererFramebuffer.Reset();
		
		m_colorAttachment = CreateColorAttachment(FBFormats::HDRColor, width, height,
		                                          VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		
		m_depthAttachment = CreateDepthAttachment(width, height, VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		
		m_waterColorAttachment = CreateColorAttachment(FBFormats::HDRColor, width, height);
		
		m_waterDepthAttachment = CreateDepthAttachment(width, height);
		
		// ** Creates the god rays attachments **
		uint32_t godRaysWidth = width / SkyRenderer::GodRaysDownscale;
		uint32_t godRaysHeight = height / SkyRenderer::GodRaysDownscale;
		m_unblurredGodRaysAttachment = CreateColorAttachment(FBFormats::GodRays, godRaysWidth, godRaysHeight);
		m_blurredGodRaysAttachment = CreateColorAttachment(FBFormats::GodRays, width, height);
		
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
		
		const VkImageView rendererAttachments[] =
		{
			*m_colorAttachment.m_imageView,
			*m_depthAttachment.m_imageView
		};
		m_rendererFramebuffer = CreateFramebuffer(renderPasses.m_renderer, rendererAttachments, width, height);
		
		const VkImageView godRaysGenAttachments[] =
		{
			*m_unblurredGodRaysAttachment.m_imageView
		};
		m_godRaysGenFramebuffer = CreateFramebuffer(renderPasses.m_godRays, godRaysGenAttachments,
		                                            godRaysWidth, godRaysHeight);
		
		const VkImageView godRaysBlurAttachments[] =
		{
			*m_blurredGodRaysAttachment.m_imageView
		};
		m_godRaysBlurFramebuffer = CreateFramebuffer(renderPasses.m_godRays, godRaysBlurAttachments, width, height);
		
		const VkImageView skyAttachments[] =
		{
			*m_colorAttachment.m_imageView
		};
		m_skyFramebuffer = CreateFramebuffer(renderPasses.m_sky, skyAttachments, width, height);
		
		const VkImageView waterAttachments[] =
		{
			*m_waterColorAttachment.m_imageView,
			*m_waterDepthAttachment.m_imageView
		};
		m_waterFramebuffer = CreateFramebuffer(renderPasses.m_water, waterAttachments, width, height);
		
		m_width = width;
		m_height = height;
	}
}
