#include "blockstexturemanager.h"
#include "blocktextures.h"
#include "texturepack.h"

#include <fstream>
#include <sstream>
#include <stb_image.h>
#include <zip.h>

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
	std::unique_ptr<BlocksTextureManager> BlocksTextureManager::s_instance;
	
	BlocksTextureManager BlocksTextureManager::LoadTexturePack(const fs::path& texturePackPath,
	                                                           LoadContext& loadContext)
	{
		TexturePack texturePack(texturePackPath);
		
		std::vector<TexturePack::Texture> textures;
		textures.reserve(GetBlockTexturesList().size() * 2);
		
		BlocksTextureManager texturesManager;
		texturesManager.m_textures.reserve(GetBlockTexturesList().size());
		
		uint32_t resolution = 0;
		
		for (const BlockTextureDesc& textureDesc : GetBlockTexturesList())
		{
			//Loads the albedo texture
			std::ostringstream albedoPathStream;
			albedoPathStream << "assets/minecraft/textures/blocks/" << textureDesc.m_name << ".png";
			TexturePack::Texture albedoTexture = texturePack.LoadTexture(albedoPathStream.str());
			
			if (albedoTexture.GetData() == nullptr)
			{
				std::ostringstream messageStream;
				messageStream << "Missing texture '" << textureDesc.m_name << "'.";
				throw std::runtime_error(messageStream.str());
			}
			
			if (textureDesc.m_gray)
			{
				albedoTexture.MultiplyColor(glm::vec4(110.0f / 255.0f, 180.0f / 255.0f, 80.0f / 255.0f, 1.0f));
			}
			
			//Loads the normal map texture
			std::ostringstream normalPathStream;
			normalPathStream << "assets/minecraft/textures/blocks/" << textureDesc.m_name << "_n.png";
			TexturePack::Texture normalTexture = texturePack.LoadTexture(normalPathStream.str());
			
			if (resolution == 0)
			{
				resolution = albedoTexture.GetWidth();
			}
			
			//Checks the resoltuion of the textures
			if (albedoTexture.GetWidth() != albedoTexture.GetHeight() ||
			    albedoTexture.GetWidth() != static_cast<int>(resolution))
			{
				throw std::runtime_error("Inconsistent texture resolution");
			}
			
			//Adds a texture entry
			texturesManager.m_textures.emplace_back();
			TextureEntry& texture = texturesManager.m_textures.back();
			texture.m_textureName = textureDesc.m_name;
			
			//Assigns the albedo texture
			texture.m_albedoTextureIndex = textures.size();
			textures.push_back(std::move(albedoTexture));
			
			//Assigns the normal map
			if (normalTexture.GetData() != nullptr)
			{
				if (normalTexture.GetWidth() != normalTexture.GetHeight() ||
				    normalTexture.GetWidth() != static_cast<int>(resolution))
				{
					throw std::runtime_error("Inconsistent texture resolution");
				}
				
				texture.m_normalTextureIndex = textures.size();
				textures.push_back(std::move(normalTexture));
			}
			else
			{
				texture.m_normalTextureIndex = -1;
			}
		}
		
		const uint32_t mipLevels = ctz(resolution);
		const uint64_t bytesPerLayer = resolution * resolution * 4;
		
		const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
		
		// ** Creates the image array **
		VkImageCreateInfo imageCreateInfo;
		InitImageCreateInfo(imageCreateInfo, VK_IMAGE_TYPE_2D, format, resolution, resolution);
		imageCreateInfo.arrayLayers = textures.size();
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
		                     bytesPerLayer * textures.size());
		
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
		for (size_t i = 0; i < textures.size(); i++)
		{
			std::copy_n(textures[i].GetData(), bytesPerLayer, stagingBufferData);
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
			/* imageExtent       */ { resolution, resolution, 1 }
		};
		loadContext.GetCB().CopyBufferToImage(stagingBuffer, *texturesManager.m_image, copyRegion);
		
		// ** Generates mip maps **
		int32_t mipSrcResolution = static_cast<int32_t>(resolution);
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
			/* mipmapMode              */ VK_SAMPLER_MIPMAP_MODE_LINEAR,
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
	
	void BlocksTextureManager::GetTextureIndices(std::string_view name, int& albedo, int& normal) const
	{
		auto it = std::find_if(MAKE_RANGE(m_textures), [&] (const TextureEntry& texture)
		{
			return texture.m_textureName == name;
		});
		
		if (it == m_textures.end())
		{
			albedo = normal = -1;
		}
		else
		{
			albedo = it->m_albedoTextureIndex;
			normal = it->m_normalTextureIndex;
		}
	}
}
