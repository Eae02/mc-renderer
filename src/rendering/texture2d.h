#pragma once

#include "../vulkan/vk.h"

namespace MCR
{
	class Texture2D
	{
	public:
		Texture2D(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageUsageFlags usageFlags);
		
		void EnqueueUpload(CommandBuffer& commandBuffer, VkBuffer sourceBuffer, uint64_t bufferOffset);
		
		void GenerateMipMaps(CommandBuffer& commandBuffer);
		
		static Texture2D FromFile(class LoadContext& loadContext, const fs::path& path, int components, bool sRGB,
		                          bool genMipMaps = false, VkImageUsageFlags additionalUsageFlags = 0);
		
		inline VkImage GetImage() const
		{ return *m_image; }
		inline VkImageView GetImageView() const
		{ return *m_imageView; }
		
		inline uint32_t GetWidth() const
		{ return m_width; }
		inline uint32_t GetHeight() const
		{ return m_height; }
		inline uint32_t GetMipLevels() const
		{ return m_mipLevels; }
		
	private:
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_mipLevels;
		
		VkHandle<VmaAllocation> m_allocation;
		VkHandle<VkImage> m_image;
		VkHandle<VkImageView> m_imageView;
	};
}
