#include "blockstexturemanager.h"

#include <fstream>
#include <sstream>
#include <stb_image.h>

#ifdef _MSC_VER
#include <intrin.h>

uint32_t __inline ctz(uint32_t value)
{
	unsigned long trailingZero;
	_BitScanForward(&trailingZero, value);
	return trailingZero;
}
#endif

#ifdef __GNUC__
#define ctz __builtin_ctz
#endif

namespace MCR
{
	constexpr uint32_t BlocksTextureManager::TextureResolution;
	
	std::unique_ptr<BlocksTextureManager> BlocksTextureManager::s_instance;
	
	struct STBIDeleter
	{
		inline void operator()(void* data) const
		{
			stbi_image_free(data);
		}
	};
	
	BlocksTextureManager BlocksTextureManager::LoadTextures(const fs::path& listPath, LoadContext& loadContext)
	{
		std::ifstream stream(listPath);
		
		if (!stream)
		{
			throw std::runtime_error("Error opening block textures list file.");
		}
		
		BlocksTextureManager texturesManager;
		std::vector<std::unique_ptr<uint8_t, STBIDeleter>> texturesMemory;
		
		fs::path parentPath = listPath.parent_path();
		
		std::string line;
		while (std::getline(stream, line))
		{
			if (line.empty() || line[0] == '#')
				continue;
			
			fs::path texturePath = parentPath / line;
			std::string texturePathString = texturePath.string();
			
			int width, height;
			stbi_uc* memory = stbi_load(texturePathString.c_str(), &width, &height, nullptr, 4);
			
			if (width != TextureResolution || height != TextureResolution)
			{
				std::ostringstream errorStream;
				errorStream << "Block texture '" << texturePathString << "' has incorrect resolution (" <<
				               width << "x" << height << ").";
				throw std::runtime_error(errorStream.str());
			}
			
			if (memory == nullptr)
			{
				std::ostringstream errorStream;
				errorStream << "Error loading block texture '" << texturePathString << "' " << stbi_failure_reason();
				throw std::runtime_error(errorStream.str());
			}
			
			texturesManager.m_textureNames.push_back(line);
			texturesMemory.emplace_back(reinterpret_cast<uint8_t*>(memory));
		}
		
		const uint32_t mipLevels = ctz(TextureResolution);
		const uint64_t bytesPerLayer = TextureResolution * TextureResolution * 4;
		
		const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
		
		// ** Creates the image array **
		VkImageCreateInfo imageCreateInfo;
		InitImageCreateInfo(imageCreateInfo, VK_IMAGE_TYPE_2D, format, TextureResolution, TextureResolution);
		imageCreateInfo.arrayLayers = texturesManager.m_textureNames.size();
		imageCreateInfo.mipLevels = mipLevels;
		imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		
		const VmaMemoryRequirements imageMemoryRequirements = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
		CheckResult(vmaCreateImage(vulkan.allocator, &imageCreateInfo, &imageMemoryRequirements,
		                           texturesManager.m_image.GetCreateAddress(),
		                           texturesManager.m_imageAllocation.GetCreateAddress(), nullptr));
		
		// ** Creates the staging buffer **
		VkBufferCreateInfo stagingBufferCreateInfo;
		InitBufferCreateInfo(stagingBufferCreateInfo, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		                     bytesPerLayer * texturesManager.m_textureNames.size());
		
		VmaAllocation stagingBufferAllocation;
		
		const VmaMemoryRequirements stagingBufferMemoryRequirements =
		{
			VMA_MEMORY_REQUIREMENT_PERSISTENT_MAP_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY
		};
		
		VmaAllocationInfo stagingBufferAllocationInfo;
		VkBuffer stagingBuffer;
		CheckResult(vmaCreateBuffer(vulkan.allocator, &stagingBufferCreateInfo, &stagingBufferMemoryRequirements,
		                            &stagingBuffer, &stagingBufferAllocation, &stagingBufferAllocationInfo));
		
		uint8_t* stagingBufferData = reinterpret_cast<uint8_t*>(stagingBufferAllocationInfo.pMappedData);
		
		// ** Copies data to the staging buffer **
		for (size_t i = 0; i < texturesMemory.size(); i++)
		{
			std::copy_n(texturesMemory[i].get(), bytesPerLayer, stagingBufferData);
			stagingBufferData += bytesPerLayer;
		}
		
		// ** Transitions the whole image to TRANSFER_DST_OPTIMAL **
		VkImageMemoryBarrier wholeImageBarrier;
		InitImageMemoryBarrier(wholeImageBarrier, *texturesManager.m_image);
		wholeImageBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, imageCreateInfo.arrayLayers };
		wholeImageBarrier.srcAccessMask = 0;
		wholeImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		wholeImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		wholeImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		
		loadContext.GetCB().PipelineBarrier(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
		                                    { }, { }, SingleElementSpan(wholeImageBarrier));
		
		// ** Uploads data to the image **
		const VkBufferImageCopy copyRegion = 
		{
			/* bufferOffset      */ 0,
			/* bufferRowLength   */ 0,
			/* bufferImageHeight */ 0,
			/* imageSubresource  */ { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, imageCreateInfo.arrayLayers },
			/* imageOffset       */ { 0, 0, 0 },
			/* imageExtent       */ { TextureResolution, TextureResolution, 1 }
		};
		loadContext.GetCB().CopyBufferToImage(stagingBuffer, *texturesManager.m_image, copyRegion);
		
		// ** Generates mip maps **
		int32_t mipSrcResolution = static_cast<int32_t>(TextureResolution);
		for (uint32_t i = 0; true; i++)
		{
			// ** Transitions the source mip level to TRANSFER_SRC_OPTIMAL **
			VkImageMemoryBarrier preBlitBarrier;
			InitImageMemoryBarrier(preBlitBarrier, *texturesManager.m_image);
			preBlitBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, i, 1, 0, imageCreateInfo.arrayLayers };
			preBlitBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			preBlitBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			preBlitBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			preBlitBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			
			loadContext.GetCB().PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			                                    { }, { }, SingleElementSpan(preBlitBarrier));
			
			if (i == mipLevels - 1)
				break;
			
			int32_t mipDstResolution = mipSrcResolution / 2;
			
			const VkImageBlit blit = 
			{
				/* srcSubresource */ { VK_IMAGE_ASPECT_COLOR_BIT, i, 0, imageCreateInfo.arrayLayers },
				/* srcOffsets     */ { { 0, 0, 0 }, { mipSrcResolution, mipSrcResolution, 1 } },
				/* dstSubresource */ { VK_IMAGE_ASPECT_COLOR_BIT, i + 1, 0, imageCreateInfo.arrayLayers },
				/* dstOffsets[2]  */ { { 0, 0, 0 }, { mipDstResolution, mipDstResolution, 1 } }
			};
			
			loadContext.GetCB().BlitImage(*texturesManager.m_image, *texturesManager.m_image, blit, VK_FILTER_LINEAR);
			
			mipSrcResolution = mipDstResolution;
		}
		
		// ** Transitions the whole image to SHADER_READ_ONLY_OPTIMAL
		wholeImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		wholeImageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		wholeImageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		wholeImageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		
		loadContext.GetCB().PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		                                    { }, { }, SingleElementSpan(wholeImageBarrier));
		
		loadContext.TakeResource(VkHandle<VkBuffer>(stagingBuffer));
		loadContext.TakeResource(VkHandle<VmaAllocation>(stagingBufferAllocation));
		
		// ** Creates the image view **
		const VkImageViewCreateInfo imageViewCreateInfo = 
		{
			/* sType            */ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			/* pNext            */ nullptr,
			/* flags            */ 0,
			/* image            */ *texturesManager.m_image,
			/* viewType         */ VK_IMAGE_VIEW_TYPE_2D_ARRAY,
			/* format           */ format,
			/* components       */ IdentityComponentMapping,
			/* subresourceRange */ { VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, imageCreateInfo.arrayLayers }
		};
		
		VkImageView imageView;
		CheckResult(vkCreateImageView(vulkan.device, &imageViewCreateInfo, nullptr, &imageView));
		texturesManager.m_imageView = imageView;
		
		// ** Creates the sampler **
		const VkSamplerCreateInfo samplerCreateInfo = 
		{
			/* sType                   */ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			/* pNext                   */ nullptr,
			/* flags                   */ 0,
			/* magFilter               */ VK_FILTER_NEAREST,
			/* minFilter               */ VK_FILTER_LINEAR,
			/* mipmapMode              */ VK_SAMPLER_MIPMAP_MODE_NEAREST,
			/* addressModeU            */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			/* addressModeV            */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			/* addressModeW            */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			/* mipLodBias              */ 0.0f,
			/* anisotropyEnable        */ VK_FALSE,
			/* maxAnisotropy           */ 0,
			/* compareEnable           */ VK_FALSE,
			/* compareOp               */ VK_COMPARE_OP_ALWAYS,
			/* minLod                  */ 0.0f,
			/* maxLod                  */ static_cast<float>(mipLevels),
			/* borderColor             */ VK_BORDER_COLOR_INT_OPAQUE_BLACK,
			/* unnormalizedCoordinates */ VK_FALSE
		};
		
		VkSampler sampler;
		CheckResult(vkCreateSampler(vulkan.device, &samplerCreateInfo, nullptr, &sampler));
		texturesManager.m_sampler = sampler;
		
		return texturesManager;
	}
	
	int BlocksTextureManager::FindTextureIndex(std::string_view name) const
	{
		for (int i = 0; i < static_cast<int>(m_textureNames.size()); i++)
		{
			if (m_textureNames[i] == name)
				return i;
		}
		return -1;
	}
}
