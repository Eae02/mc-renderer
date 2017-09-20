#pragma once

#include "../vulkan/vk.h"
#include "../loadcontext.h"

namespace MCR
{
	class UIImage
	{
	public:
		static UIImage FromFile(const fs::path& path, LoadContext& loadContext);
		
		static UIImage CreateSingleColor(const glm::vec4& color, uint32_t width, uint32_t height, CommandBuffer& cb);
		
		inline VkDescriptorSet GetDescriptorSet() const
		{
			return *m_descriptorSet;
		}
		
	private:
		UIImage(uint32_t width, uint32_t height);
		
		void BeforeInitialTransfer(CommandBuffer& cb);
		void AfterInitialTransfer(CommandBuffer& cb);
		
		uint32_t m_width;
		uint32_t m_height;
		
		VkHandle<VmaAllocation> m_allocation;
		VkHandle<VkImage> m_image;
		VkHandle<VkImageView> m_imageView;
		UniqueDescriptorSet m_descriptorSet;
	};
}
