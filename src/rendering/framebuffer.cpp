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
	
	Framebuffer::Attachment Framebuffer::CreateDepthAttachment(uint32_t width, uint32_t height,
	                                                           VkImageUsageFlags extraUsageFlags)
	{
		Attachment attachment;
		
		//Creates the image
		VkImageCreateInfo imageCreateInfo;
		InitImageCreateInfo(imageCreateInfo, VK_IMAGE_TYPE_2D, vulkan.depthFormat, width, height);
		imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | extraUsageFlags;
		
		CheckResult(vmaCreateImage(vulkan.allocator, &imageCreateInfo, &gpuAllocationCI,
		                           attachment.m_image.GetCreateAddress(),
		                           attachment.m_allocation.GetCreateAddress(), nullptr));
		
		//Creates the image view
		VkImageViewCreateInfo viewCreateInfo;
		InitImageViewCreateInfo(viewCreateInfo, *attachment.m_image, VK_IMAGE_VIEW_TYPE_2D,
		                        vulkan.depthFormat, { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 });
		CheckResult(vkCreateImageView(vulkan.device, &viewCreateInfo, nullptr,
		                              attachment.m_imageView.GetCreateAddress()));
		
		return attachment;
	}
	
	void Framebuffer::Create(const RenderPasses& renderPasses, gsl::span<const VkImage> outputImages,
	                         uint32_t width, uint32_t height)
	{
		m_rendererFramebuffer.Reset();
		
		m_colorAttachment = CreateColorAttachment(Renderer::ColorAttachmentFormat, width, height,
		                                          VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		
		m_depthAttachment = CreateDepthAttachment(width, height, VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		
		m_waterColorAttachment = CreateColorAttachment(Renderer::ColorAttachmentFormat, width, height,
		                                               VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		
		m_waterDepthAttachment = CreateDepthAttachment(width, height, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		
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
		
		const VkImageView rendererAttachments[] =
		{
			*m_colorAttachment.m_imageView,
			*m_depthAttachment.m_imageView
		};
		m_rendererFramebuffer = CreateFramebuffer(renderPasses.m_renderer, rendererAttachments, width, height);
		
		const VkImageView waterAttachments[] =
		{
			*m_waterColorAttachment.m_imageView,
			*m_waterDepthAttachment.m_imageView
		};
		m_waterFramebuffer = CreateFramebuffer(renderPasses.m_water, waterAttachments, width, height);
		
		m_godRaysGenFramebuffer =
		        CreateFramebuffer(renderPasses.m_godRays, SingleElementSpan(*m_unblurredGodRaysAttachment.m_imageView),
		                          godRaysWidth, godRaysHeight);
		
		m_godRaysBlurFramebuffer = 
		        CreateFramebuffer(renderPasses.m_godRays, SingleElementSpan(*m_blurredGodRaysAttachment.m_imageView),
		                          width, height);
		
		m_width = width;
		m_height = height;
	}
	
	void Framebuffer::EnqueueWaterCopyCommands(CommandBuffer& commandBuffer) const
	{
		// ** Inserts barriers preparing water output attachments for being copied to **
		VkImageMemoryBarrier preCopyBarriers[2];
		
		InitImageMemoryBarrier(preCopyBarriers[0], *m_waterColorAttachment.m_image);
		preCopyBarriers[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		preCopyBarriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		preCopyBarriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		
		InitImageMemoryBarrier(preCopyBarriers[1], *m_waterDepthAttachment.m_image);
		preCopyBarriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		preCopyBarriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		preCopyBarriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		preCopyBarriers[1].subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
		
		commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		                              0, { }, { }, preCopyBarriers);
		
		// ** Copies the water input attachments to the water output attachments **
		const VkImageCopy colorCopyRegion =
		{
			/* srcSubresource */ { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
			/* srcOffset      */ { 0, 0, 0 },
			/* dstSubresource */ { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
			/* dstOffset      */ { 0, 0, 0 },
			/* extent         */ { m_width, m_height, 1 }
		};
		commandBuffer.CopyImage(*m_colorAttachment.m_image, *m_waterColorAttachment.m_image, colorCopyRegion);
		
		const VkImageCopy depthCopyRegion =
		{
			/* srcSubresource */ { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 0, 1 },
			/* srcOffset      */ { 0, 0, 0 },
			/* dstSubresource */ { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 0, 1 },
			/* dstOffset      */ { 0, 0, 0 },
			/* extent         */ { m_width, m_height, 1 }
		};
		commandBuffer.CopyImage(*m_depthAttachment.m_image, *m_waterDepthAttachment.m_image, depthCopyRegion);
		
		// ** Inserts barriers preparing water input attachments for reading in the fragment shader **
		VkImageMemoryBarrier postCopyBarriers[2];
		
		InitImageMemoryBarrier(postCopyBarriers[0], *m_colorAttachment.m_image);
		postCopyBarriers[0].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		postCopyBarriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		postCopyBarriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		postCopyBarriers[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		
		InitImageMemoryBarrier(postCopyBarriers[1], *m_depthAttachment.m_image);
		postCopyBarriers[1].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		postCopyBarriers[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		postCopyBarriers[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		postCopyBarriers[1].newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		postCopyBarriers[1].subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
		
		commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		                              0, { }, { }, postCopyBarriers);
	}
}
