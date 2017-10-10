#pragma once

#include "../vulkan/vk.h"

#include <memory>

namespace MCR
{
	class WindNoiseImage
	{
	public:
		WindNoiseImage(uint32_t size);
		
		static WindNoiseImage Generate(uint32_t size, class LoadContext& loadContext);
		
		inline static void SetInstance(std::unique_ptr<WindNoiseImage> instance)
		{
			s_instance = std::move(instance);
		}
		
		inline static WindNoiseImage& GetInstance()
		{
			return *s_instance;
		}
		
		inline VkImageView GetImageView() const
		{
			return *m_imageView;
		}
		
		inline VkDescriptorImageInfo GetImageInfo() const
		{
			return { VK_NULL_HANDLE, *m_imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		}
		
	private:
		static std::unique_ptr<WindNoiseImage> s_instance;
		
		VkHandle<VmaAllocation> m_imageAllocation;
		VkHandle<VkImage> m_image;
		VkHandle<VkImageView> m_imageView;
	};
}
