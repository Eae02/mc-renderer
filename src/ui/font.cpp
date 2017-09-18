#include "font.h"
#include "../loadcontext.h"

#include <cstring>
#include <cctype>
#include <algorithm>
#include <stdexcept>
#include <utf8.h>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace MCR
{
	static FT_Library theFTLibrary;
	
	bool Font::InitFreetype()
	{
		return FT_Init_FreeType(&theFTLibrary) == 0;
	}
	
	void Font::DestroyFreeType()
	{
		FT_Done_FreeType(theFTLibrary);
	}
	
	//Character ranges must be sorted in ascending order by code points and not overlap!
	const uint32_t characterRanges[][2] = 
	{
		{ 33, 126 }, //ASCII standard characters
		{ 161, 255 } //Latin supplement
	};
	
	Font Font::Render(const fs::path& path, uint32_t size, LoadContext& loadContext)
	{
		Font font;
		font.m_size = size;
		
		std::string pathString = path.string();
		
		// ** Opens the font **
		FT_Face fontFace;
		FT_Error state = FT_New_Face(theFTLibrary, pathString.c_str(), 0, &fontFace);
		if (state == FT_Err_Unknown_File_Format)
		{
			throw std::runtime_error("Unsupported font format.");
		}
		else if (state != 0)
		{
			throw std::runtime_error("Error reading font file.");
		}
		
		FT_Set_Pixel_Sizes(fontFace, 0, size);
		
		FT_Load_Char(fontFace, ' ', FT_LOAD_DEFAULT);
		font.m_spaceAdvance = fontFace->glyph->advance.x / 64.0f;
		
		struct GlyphImage
		{
			VkBuffer m_stagingBuffer;
			VmaAllocation m_stagingBufferAllocation;
			uint32_t m_width;
			uint32_t m_height;
		};
		
		std::vector<GlyphImage> layers;
		
		const VmaMemoryRequirements stagingBufferRequirements =
		{
			VMA_MEMORY_REQUIREMENT_PERSISTENT_MAP_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY
		};
		
		uint32_t maxWidth = 0;
		uint32_t maxHeight = 0;
		
		// ** Renders characters and copies the glyphs into staging buffers **
		for (const uint32_t* charRange : characterRanges)
		{
			for (uint32_t c = charRange[0]; c <= charRange[1]; c++)
			{
				if (FT_Load_Char(fontFace, c, FT_LOAD_RENDER))
					continue;
				
				if (fontFace->glyph->bitmap.width == 0 || fontFace->glyph->bitmap.rows == 0)
					continue;
				
				if (c == '*')
				{
					font.m_defaultGlyphIndex = font.m_glyphs.size();
				}
				
				const uint32_t width = fontFace->glyph->bitmap.width;
				const uint32_t height = fontFace->glyph->bitmap.rows;
				
				font.m_glyphs.emplace_back();
				font.m_glyphs.back().m_charCode = c;
				font.m_glyphs.back().m_advance = fontFace->glyph->advance.x / 64.0f;
				font.m_glyphs.back().m_bearingX = fontFace->glyph->bitmap_left;
				font.m_glyphs.back().m_bearingY = fontFace->glyph->bitmap_top;
				font.m_glyphs.back().m_size = { width, height };
				font.m_glyphs.back().m_texLayer = static_cast<float>(layers.size());
				
				if (width > maxWidth)
					maxWidth = width;
				if (height > maxHeight)
					maxHeight = height;
				
				const uint64_t bufferSize = width * height;
				
				layers.emplace_back();
				layers.back().m_width = width;
				layers.back().m_height = height;
				
				VkBufferCreateInfo stagingBufferCreateInfo;
				InitBufferCreateInfo(stagingBufferCreateInfo, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, bufferSize);
				
				VmaAllocationInfo allocationInfo;
				
				CheckResult(vmaCreateBuffer(vulkan.allocator, &stagingBufferCreateInfo, &stagingBufferRequirements,
				                            &layers.back().m_stagingBuffer, &layers.back().m_stagingBufferAllocation,
				                            &allocationInfo));
				
				std::memcpy(allocationInfo.pMappedData, fontFace->glyph->bitmap.buffer, bufferSize);
			}
		}
		
		for (Glyph& glyph : font.m_glyphs)
		{
			glyph.m_maxTexCoord = glyph.m_size / glm::vec2(maxWidth, maxHeight);
		}
		
		// ** Creates the image array **
		VkImageCreateInfo imageCreateInfo;
		InitImageCreateInfo(imageCreateInfo, VK_IMAGE_TYPE_2D, VK_FORMAT_R8_UNORM, maxWidth, maxHeight);
		imageCreateInfo.arrayLayers = gsl::narrow<uint32_t>(font.m_glyphs.size());
		imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		
		VkImage image;
		VmaAllocation imageAllocation;
		
		const VmaMemoryRequirements imageRequirements = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
		CheckResult(vmaCreateImage(vulkan.allocator, &imageCreateInfo, &imageRequirements, &image,
		                           &imageAllocation, nullptr));
		
		font.m_image = image;
		font.m_imageAllocation = imageAllocation;
		
		// ** Creates the image view **
		const VkImageViewCreateInfo imageViewCreateInfo = 
		{
			/* sType            */ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			/* pNext            */ nullptr,
			/* flags            */ 0,
			/* image            */ image,
			/* viewType         */ VK_IMAGE_VIEW_TYPE_2D_ARRAY,
			/* format           */ imageCreateInfo.format,
			/* components       */ { VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_ONE,
			                         VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_R },
			/* subresourceRange */ { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, imageCreateInfo.arrayLayers }
		};
		
		VkImageView imageView;
		CheckResult(vkCreateImageView(vulkan.device, &imageViewCreateInfo, nullptr, &imageView));
		font.m_imageView = imageView;
		
		VkWriteDescriptorSet writeDescriptorSet;
		const VkDescriptorImageInfo imageInfo = { VK_NULL_HANDLE, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		font.m_descriptorSet.InitWriteDescriptorSet(writeDescriptorSet, 0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, imageInfo);
		UpdateDescriptorSets(SingleElementSpan(writeDescriptorSet));
		
		// ** Uploads glyphs to the image array **
		VkImageMemoryBarrier imageBarrier;
		InitImageMemoryBarrier(imageBarrier, image);
		imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier.subresourceRange = imageViewCreateInfo.subresourceRange;
		
		loadContext.GetCB().PipelineBarrier(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
		                                    { }, { }, SingleElementSpan(imageBarrier));
		
		for (size_t i = 0; i < layers.size(); i++)
		{
			const VkBufferImageCopy copyRegion = 
			{
				/* bufferOffset      */ 0,
				/* bufferRowLength   */ 0,
				/* bufferImageHeight */ 0,
				/* imageSubresource  */ { VK_IMAGE_ASPECT_COLOR_BIT, 0, gsl::narrow<uint32_t>(i), 1 },
				/* imageOffset       */ { 0, 0, 0 },
				/* imageExtent       */ { layers[i].m_width, layers[i].m_height, 1 }
			};
			
			loadContext.GetCB().CopyBufferToImage(layers[i].m_stagingBuffer, image, copyRegion);
			
			loadContext.TakeResource(VkHandle<VkBuffer>(layers[i].m_stagingBuffer));
			loadContext.TakeResource(VkHandle<VmaAllocation>(layers[i].m_stagingBufferAllocation));
		}
		
		imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		loadContext.GetCB().PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		                                    { }, { }, SingleElementSpan(imageBarrier));
		
		return font;
	}
	
	glm::vec2 Font::GetTextExtents(std::string_view text) const
	{
		int maxLineWidth = 0;
		int currentLineWidth = 0;
		
		int numLines = 1;
		
		for (auto it = text.begin(); it != text.end();)
		{
			const uint32_t charCode = utf8::next(it, text.end());
			if (charCode == 0)
				continue;
			
			if (charCode == '\n')
			{
				if (maxLineWidth > currentLineWidth)
					maxLineWidth = currentLineWidth;
				numLines++;
				
				currentLineWidth = 0;
				continue;
			}
			
			if (charCode == ' ')
			{
				currentLineWidth += m_spaceAdvance;
				continue;
			}
			
			if (::isspace(charCode))
				continue;
			
			const Glyph& glyph = FindGlyph(charCode);
			currentLineWidth += glyph.m_advance;
		}
		
		return { std::max(maxLineWidth, currentLineWidth), numLines * m_size };
	}
	
	const Font::Glyph& Font::FindGlyph(uint32_t charCode) const
	{
		auto glyphIt = std::lower_bound(MAKE_RANGE(m_glyphs), charCode, [] (const Glyph& glyph, uint32_t c)
		{
			return glyph.m_charCode < c;
		});
		
		if (glyphIt == m_glyphs.end() || glyphIt->m_charCode != charCode)
		{
			return m_glyphs[m_defaultGlyphIndex];
		}
		
		return *glyphIt;
	}
}
