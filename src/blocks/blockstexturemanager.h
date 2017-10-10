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
		static BlocksTextureManager LoadTexturePack(const fs::path& texturePackPath, LoadContext& loadContext);
		
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
		
		int GetAlbedoTextureIndex(std::string_view name) const
		{
			int albedo, normal;
			GetTextureIndices(name, albedo, normal);
			return albedo;
		}
		
		void GetTextureIndices(std::string_view name, int& albedo, int& normal) const;
		
	private:
		static std::unique_ptr<BlocksTextureManager> s_instance;
		
		BlocksTextureManager() = default;
		
		struct TextureEntry
		{
			std::string_view m_textureName;
			int m_albedoTextureIndex;
			int m_normalTextureIndex;
		};
		
		std::vector<TextureEntry> m_textures;
		
		VkHandle<VkImage> m_image;
		VkHandle<VmaAllocation> m_imageAllocation;
		
		VkHandle<VkImageView> m_imageView;
		VkHandle<VkSampler> m_sampler;
	};
}
