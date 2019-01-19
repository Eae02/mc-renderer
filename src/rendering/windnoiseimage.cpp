#include "windnoiseimage.h"
#include "../loadcontext.h"

#include <libnoise/module/perlin.h>

namespace MCR
{
	std::unique_ptr<WindNoiseImage> WindNoiseImage::s_instance;
	
	WindNoiseImage::WindNoiseImage(uint32_t size)
	{
		const VmaAllocationCreateInfo allocationCI = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
		
		VkImageCreateInfo imageCreateInfo;
		InitImageCreateInfo(imageCreateInfo, VK_IMAGE_TYPE_1D, VK_FORMAT_R32_SFLOAT, size, 1);
		imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		
		CheckResult(vmaCreateImage(vulkan.allocator, &imageCreateInfo, &allocationCI, m_image.GetCreateAddress(),
		                           m_imageAllocation.GetCreateAddress(), nullptr));
		
		VkImageViewCreateInfo imageViewCreateInfo;
		InitImageViewCreateInfo(imageViewCreateInfo, *m_image, VK_IMAGE_VIEW_TYPE_1D, imageCreateInfo.format,
		                        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		
		CheckResult(vkCreateImageView(vulkan.device, &imageViewCreateInfo, nullptr, m_imageView.GetCreateAddress()));
	}
	
	WindNoiseImage WindNoiseImage::Generate(uint32_t size, LoadContext& loadContext)
	{
		WindNoiseImage image(size);
		
		//Creates a staging buffer for noise values
		const VmaAllocationCreateInfo bufferAllocationCI =
		{
			VMA_ALLOCATION_CREATE_MAPPED_BIT, 
			VMA_MEMORY_USAGE_CPU_ONLY
		};
		
		VkBufferCreateInfo bufferCreateInfo;
		InitBufferCreateInfo(bufferCreateInfo, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, sizeof(float) * size);
		
		VkBuffer buffer;
		VmaAllocation bufferAllocation;
		VmaAllocationInfo bufferAllocationInfo;
		CheckResult(vmaCreateBuffer(vulkan.allocator, &bufferCreateInfo, &bufferAllocationCI, &buffer,
		                            &bufferAllocation, &bufferAllocationInfo));
		
		float* bufferData = reinterpret_cast<float*>(bufferAllocationInfo.pMappedData);
		
		//Generates noise values
		noise::module::Perlin perlin;
		perlin.SetSeed(0);
		for (uint32_t x = 0; x < size; x++)
		{
			bufferData[x] = perlin.GetValue(static_cast<double>(x) / static_cast<double>(size), 0, 0);
		}
		
		//Transitions the image to TRANSFER_DST_OPTIMAL
		VkImageMemoryBarrier preUploadImageBarrier;
		InitImageMemoryBarrier(preUploadImageBarrier, *image.m_image);
		preUploadImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		preUploadImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		preUploadImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		
		loadContext.GetCB().PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
		                                    { }, { }, SingleElementSpan(preUploadImageBarrier));
		
		//Uploads the noise values
		const VkBufferImageCopy copyRegion = 
		{
			/* bufferOffset      */ 0,
			/* bufferRowLength   */ 0,
			/* bufferImageHeight */ 0,
			/* imageSubresource  */ { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
			/* imageOffset       */ { 0, 0, 0 },
			/* imageExtent       */ { size, 1, 1 }
		};
		loadContext.GetCB().CopyBufferToImage(buffer, *image.m_image, copyRegion);
		
		//Transitions the image to SHADER_READ_ONLY_OPTIMAL
		VkImageMemoryBarrier postUploadImageBarrier;
		InitImageMemoryBarrier(postUploadImageBarrier, *image.m_image);
		postUploadImageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		postUploadImageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		postUploadImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		postUploadImageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		
		loadContext.GetCB().PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0,
		                                    { }, { }, SingleElementSpan(postUploadImageBarrier));
		
		loadContext.TakeResource(VkHandle<VkBuffer>(buffer));
		loadContext.TakeResource(VkHandle<VmaAllocation>(bufferAllocation));
		
		return image;
	}
}
