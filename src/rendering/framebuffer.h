#pragma once

#include "../vulkan/vk.h"

#include <array>

namespace MCR
{
	class Framebuffer
	{
	public:
		Framebuffer() = default;
		
		void Create(const class Renderer& renderer, const class UIGraphicsContext& uiGraphicsContext,
		            uint32_t width, uint32_t height);
		
		inline VkFramebuffer GetRendererFramebuffer() const
		{ return *m_rendererFramebuffer; }
		
		inline VkFramebuffer GetUIFramebuffer() const
		{ return *m_uiFramebuffer; }
		
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
		
		inline VkImage GetColorImage() const
		{
			return *m_colorAttachment.m_image;
		}
		
		inline VkImage GetDepthImage() const
		{
			return *m_depthAttachment.m_image;
		}
		
		inline VkImageView GetColorImageView() const
		{
			return *m_colorAttachment.m_imageView;
		}
		
		inline VkImageView GetDepthImageView() const
		{
			return *m_depthAttachment.m_imageView;
		}
		
		void GetPresentImage(SwapChain::PresentImage& presentImageOut) const;
		
	private:
		struct Attachment
		{
			VkHandle<VkImage> m_image;
			VkHandle<VkImageView> m_imageView;
			VkHandle<VmaAllocation> m_allocation;
		};
		
		Attachment m_colorAttachment;
		Attachment m_depthAttachment;
		
		VkHandle<VkFramebuffer> m_rendererFramebuffer;
		VkHandle<VkFramebuffer> m_uiFramebuffer;
		
		uint32_t m_width;
		uint32_t m_height;
	};
}
