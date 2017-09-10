#pragma once

#include "../vulkan/vkhandle.h"
#include "../loadcontext.h"
#include "../filesystem.h"

#include <vector>
#include <string>

namespace MCR
{
	class BlocksTextureManager
	{
	public:
		static BlocksTextureManager LoadTextures(const fs::path& listPath, LoadContext& loadContext);
		
		static constexpr uint32_t TextureResolution = 64;
		
		inline static void SetInstance(std::unique_ptr<BlocksTextureManager> instance)
		{
			s_instance = std::move(instance);
		}
		
		inline static BlocksTextureManager& GetInstance()
		{
			return *s_instance;
		}
		
		inline VkDescriptorImageInfo GetImageInfo() const
		{
			return { *m_sampler, *m_imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		}
		
		int FindTextureIndex(const std::string& name) const;
		
	private:
		static std::unique_ptr<BlocksTextureManager> s_instance;
		
		BlocksTextureManager() = default;
		
		std::vector<std::string> m_textureNames;
		
		VkHandle<VkImage> m_image;
		VkHandle<VmaAllocation> m_imageAllocation;
		
		VkHandle<VkImageView> m_imageView;
		VkHandle<VkSampler> m_sampler;
	};
}
