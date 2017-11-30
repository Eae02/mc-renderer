#include "texture2d.h"
#include "../loadcontext.h"

#include <stb_image.h>
#include <cstring>

namespace MCR
{
	Texture2D::Texture2D(uint32_t width, uint32_t height, uint32_t mipLevels,
	                     VkFormat format, VkImageUsageFlags usageFlags)
	    : m_width(width), m_height(height), m_mipLevels(mipLevels)
	{
		// ** Creates the image **
		VkImageCreateInfo imageCreateInfo;
		InitImageCreateInfo(imageCreateInfo, VK_IMAGE_TYPE_2D, format, width, height);
		imageCreateInfo.usage = usageFlags;
		imageCreateInfo.mipLevels = mipLevels;
		
		VmaAllocationCreateInfo imageAllocationCI = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
		vmaCreateImage(vulkan.allocator, &imageCreateInfo, &imageAllocationCI, m_image.GetCreateAddress(),
		               m_allocation.GetCreateAddress(), nullptr);
		
		// ** Creates the image view **
		VkImageViewCreateInfo normalMapViewCreateInfo;
		InitImageViewCreateInfo(normalMapViewCreateInfo, *m_image, VK_IMAGE_VIEW_TYPE_2D, format,
		                        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		vkCreateImageView(vulkan.device, &normalMapViewCreateInfo, nullptr, m_imageView.GetCreateAddress());
	}
	
	static const VkFormat unormFormats[] =
	{
		VK_FORMAT_R8_UNORM, VK_FORMAT_R8G8_UNORM, VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_R8G8B8A8_UNORM
	};
	
	static const VkFormat sRGBFormats[] =
	{
		VK_FORMAT_R8_SRGB, VK_FORMAT_R8G8_SRGB, VK_FORMAT_R8G8B8_SRGB, VK_FORMAT_R8G8B8A8_SRGB
	};
	
	Texture2D Texture2D::FromFile(LoadContext& loadContext, const fs::path& path, int components, bool sRGB,
	                              bool genMipMaps, VkImageUsageFlags additionalUsageFlags)
	{
		std::string pathStr = path.string();
		
		int width, height;
		stbi_uc* imageData = stbi_load(pathStr.c_str(), &width, &height, nullptr, components);
		const VkDeviceSize imageBytes = static_cast<const VkDeviceSize>(width * height * components);
		
		const VkFormat format = (sRGB ? &sRGBFormats[0] : &unormFormats[0])[components - 1];
		
		uint32_t mipLevels = 1;
		
		if (genMipMaps)
		{
			mipLevels = CalculateMipLevels(static_cast<uint32_t>(std::min(width, height)));
			additionalUsageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}
		
		Texture2D texture(static_cast<uint32_t>(width), static_cast<uint32_t>(height), mipLevels, format,
		                  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | additionalUsageFlags);
		
		// ** Creates the staging buffer **
		VkHandle<VmaAllocation> stagingAllocation;
		VkHandle<VkBuffer> stagingBuffer;
		void* stagingBufferMemory;
		CreateStagingBuffer(imageBytes, stagingBuffer.GetCreateAddress(), stagingAllocation.GetCreateAddress(),
		                    &stagingBufferMemory);
		
		std::memcpy(reinterpret_cast<char*>(stagingBufferMemory), imageData, imageBytes);
		
		texture.EnqueueUpload(loadContext.GetCB(), *stagingBuffer, 0);
		
		if (genMipMaps)
		{
			texture.GenerateMipMaps(loadContext.GetCB());
		}
		
		loadContext.TakeResource(std::move(stagingAllocation));
		loadContext.TakeResource(std::move(stagingBuffer));
		
		return texture;
	}
	
	void Texture2D::EnqueueUpload(CommandBuffer& commandBuffer, VkBuffer sourceBuffer, uint64_t bufferOffset)
	{
		// ** Transitions the image to TRANSFER_DST_OPTIMAL **
		VkImageMemoryBarrier preCopyBarrier;
		InitImageMemoryBarrier(preCopyBarrier, *m_image);
		preCopyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		preCopyBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		preCopyBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		preCopyBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, m_mipLevels, 0, 1 };
		commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
		                              { }, { }, SingleElementSpan(preCopyBarrier));
		
		// ** Uploads image data **
		const VkBufferImageCopy copyRegion =
		{
			/* bufferOffset      */ bufferOffset,
			/* bufferRowLength   */ 0,
			/* bufferImageHeight */ 0,
			/* imageSubresource  */ { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
			/* imageOffset       */ { 0, 0, 0 },
			/* imageExtent       */ { m_width, m_height, 1 }
		};
		commandBuffer.CopyBufferToImage(sourceBuffer, *m_image, copyRegion);
		
		// ** Transitions the image to SHADER_READ_ONLY_OPTIMAL **
		VkImageMemoryBarrier postCopyBarrier;
		InitImageMemoryBarrier(postCopyBarrier, *m_image);
		postCopyBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		postCopyBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		postCopyBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		postCopyBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		postCopyBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, m_mipLevels, 0, 1 };
		commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		                              { }, { }, SingleElementSpan(postCopyBarrier));
	}
	
	void Texture2D::GenerateMipMaps(CommandBuffer& commandBuffer)
	{
		VkImageMemoryBarrier preGenBarriers[2];
		// ** Transitions the first mip level to TRANSFER_SRC_OPTIMAL **
		InitImageMemoryBarrier(preGenBarriers[0], *m_image);
		preGenBarriers[0].subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		preGenBarriers[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		preGenBarriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		preGenBarriers[0].oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		preGenBarriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		
		// ** Transitions all mip levels except the first to TRANSFER_DST_OPTIMAL **
		InitImageMemoryBarrier(preGenBarriers[1], *m_image);
		preGenBarriers[1].subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_REMAINING_MIP_LEVELS, 0, 1 };
		preGenBarriers[1].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		preGenBarriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		preGenBarriers[1].oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		preGenBarriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		
		commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
		                              { }, { }, preGenBarriers);
		
		// ** Generates mip maps **
		int32_t mipSrcWidth = static_cast<int32_t>(m_width);
		int32_t mipSrcHeight = static_cast<int32_t>(m_height);
		for (uint32_t i = 1; i < m_mipLevels; i++)
		{
			int32_t mipDstWidth = mipSrcWidth / 2;
			int32_t mipDstHeight = mipSrcWidth / 2;
			
			const VkImageBlit blit =
			{
				/* srcSubresource */ { VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, 1 },
				/* srcOffsets     */ { { 0, 0, 0 }, { mipSrcWidth, mipSrcHeight, 1 } },
				/* dstSubresource */ { VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1 },
				/* dstOffsets[2]  */ { { 0, 0, 0 }, { mipDstWidth, mipDstHeight, 1 } }
			};
			
			commandBuffer.BlitImage(*m_image, *m_image, blit, VK_FILTER_LINEAR);
			
			mipSrcWidth = mipDstWidth;
			mipSrcHeight = mipDstHeight;
			
			// ** Transitions the destination mip level to TRANSFER_SRC_OPTIMAL **
			VkImageMemoryBarrier postBlitBarrier;
			InitImageMemoryBarrier(postBlitBarrier, *m_image);
			postBlitBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, i, 1, 0, 1 };
			postBlitBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			postBlitBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			postBlitBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			postBlitBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			
			commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			                              { }, { }, SingleElementSpan(postBlitBarrier));
		}
		
		// ** Transitions all mip levels to SHADER_READ_ONLY_OPTIMAL **
		VkImageMemoryBarrier postGenBarrier;
		InitImageMemoryBarrier(postGenBarrier, *m_image);
		postGenBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		postGenBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		postGenBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		postGenBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		postGenBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		                              { }, { }, SingleElementSpan(postGenBarrier));
	}
}
