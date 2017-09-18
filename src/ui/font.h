#pragma once

#include "../utils.h"
#include "../vulkan/vk.h"

#include <string_view>
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

namespace MCR
{
	class Font
	{
	public:
		struct Glyph
		{
			uint32_t m_charCode;
			float m_advance;
			int m_bearingX;
			int m_bearingY;
			glm::vec2 m_size;
			float m_texLayer;
			glm::vec2 m_maxTexCoord;
		};
		
		static bool InitFreetype();
		static void DestroyFreeType();
		
		static Font Render(const fs::path& path, uint32_t size, class LoadContext& loadContext);
		
		glm::vec2 GetTextExtents(std::string_view text) const;
		
		const Glyph& FindGlyph(uint32_t charCode) const;
		
		inline float GetSpaceAdvance() const
		{
			return m_spaceAdvance;
		}
		
		inline uint32_t GetSize() const
		{
			return m_size;
		}
		
		inline VkDescriptorSet GetDescriptorSet() const
		{
			return *m_descriptorSet;
		}
		
	private:
		inline Font()
		    : m_descriptorSet("UI_Image") { }
		
		float m_spaceAdvance;
		std::vector<Glyph> m_glyphs;
		
		uint32_t m_size;
		
		size_t m_defaultGlyphIndex;
		
		VkHandle<VkImage> m_image;
		VkHandle<VmaAllocation> m_imageAllocation;
		
		VkHandle<VkImageView> m_imageView;
		
		UniqueDescriptorSet m_descriptorSet;
	};
}
