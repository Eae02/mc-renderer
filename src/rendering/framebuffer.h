#pragma once

#include "../vulkan/vk.h"

#include <array>

namespace MCR
{
	class Framebuffer
	{
	public:
		Framebuffer() = default;
		
		struct RenderPasses
		{
			VkRenderPass m_renderer;
			VkRenderPass m_sky;
			VkRenderPass m_godRays;
			VkRenderPass m_water;
			VkRenderPass m_ui;
		};
		
		void Create(const RenderPasses& renderPasses, gsl::span<const VkImage> outputImages,
		            uint32_t width, uint32_t height);
		
		void EnqueueWaterCopyCommands(CommandBuffer& commandBuffer) const;
		
		inline VkFramebuffer GetRendererFramebuffer() const
		{
			return *m_rendererFramebuffer;
		}
		
		inline VkFramebuffer GetSkyFramebuffer() const
		{
			return *m_skyFramebuffer;
		}
		
		inline VkFramebuffer GetGodRaysGenFramebuffer() const
		{
			return *m_godRaysGenFramebuffer;
		}
		
		inline VkFramebuffer GetGodRaysBlurFramebuffer() const
		{
			return *m_godRaysBlurFramebuffer;
		}
		
		inline VkFramebuffer GetWaterFramebuffer() const
		{
			return *m_waterFramebuffer;
		}
		
		inline VkFramebuffer GetOutputFramebuffer(size_t index) const
		{
			return *m_outputs[index].m_framebuffer;
		}
		
		inline void GetViewportAndRenderArea(VkRect2D& renderArea, VkViewport& viewport) const
		{
			renderArea.offset.x = 0;
			renderArea.offset.y = 0;
			renderArea.extent.width = m_width;
			renderArea.extent.height = m_height;
			viewport.x = 0;
			viewport.y = 0;
			viewport.width = m_width;
			viewport.height = m_height;
			viewport.minDepth = 0;
			viewport.maxDepth = 1;
		}
		
		inline uint32_t GetWidth() const
		{ return m_width; }
		inline uint32_t GetHeight() const
		{ return m_height; }
		
		inline VkImageView GetWaterColorImageView() const
		{
			return *m_waterColorAttachment.m_imageView;
		}
		
		inline VkImageView GetWaterDepthImageView() const
		{
			return *m_waterDepthAttachment.m_imageView;
		}
		
		inline VkImageView GetColorImageView() const
		{
			return *m_colorAttachment.m_imageView;
		}
		
		inline VkImageView GetDepthImageView() const
		{
			return *m_depthAttachment.m_imageView;
		}
		
		inline VkImageView GetUnblurredGodRaysImageView() const
		{
			return *m_unblurredGodRaysAttachment.m_imageView;
		}
		
		inline VkImageView GetBlurredGodRaysImageView() const
		{
			return *m_blurredGodRaysAttachment.m_imageView;
		}
		
	private:
		struct Attachment
		{
			VkHandle<VkImage> m_image;
			VkHandle<VkImageView> m_imageView;
			VkHandle<VmaAllocation> m_allocation;
		};
		
		static Attachment CreateColorAttachment(VkFormat format, uint32_t width, uint32_t height,
		                                        VkImageUsageFlags extraUsageFlags = 0);
		
		static Attachment CreateDepthAttachment(uint32_t width, uint32_t height, VkImageUsageFlags extraUsageFlags = 0);
		
		Attachment m_colorAttachment;
		Attachment m_depthAttachment;
		Attachment m_waterColorAttachment;
		Attachment m_waterDepthAttachment;
		Attachment m_unblurredGodRaysAttachment;
		Attachment m_blurredGodRaysAttachment;
		
		struct OutputEntry
		{
			VkHandle<VkImageView> m_imageView;
			VkHandle<VkFramebuffer> m_framebuffer;
		};
		
		std::vector<OutputEntry> m_outputs;
		
		VkHandle<VkFramebuffer> m_rendererFramebuffer;
		VkHandle<VkFramebuffer> m_godRaysGenFramebuffer;
		VkHandle<VkFramebuffer> m_godRaysBlurFramebuffer;
		VkHandle<VkFramebuffer> m_skyFramebuffer;
		VkHandle<VkFramebuffer> m_waterFramebuffer;
		
		uint32_t m_width;
		uint32_t m_height;
	};
}
