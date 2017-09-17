#pragma once

#include "../vulkan/vk.h"

#include <array>

namespace MCR
{
	class RendererFramebuffer
	{
	public:
		RendererFramebuffer();
		
		void Create(const class Renderer& renderer, uint32_t width, uint32_t height);
		
		inline VkFramebuffer GetFramebuffer() const
		{ return *m_framebuffer; }
		
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
		
		VkHandle<VkFramebuffer> m_framebuffer;
		
		uint32_t m_width;
		uint32_t m_height;
	};
}
