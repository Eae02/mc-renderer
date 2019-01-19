#include "uiimage.h"
#include <stb_image.h>
#include <gsl/gsl_util>
#include <cstring>

namespace MCR
{
	UIImage UIImage::FromFile(const fs::path& path, LoadContext& loadContext)
	{
		std::string pathString = path.string();
		
		int width, height;
		stbi_uc* memory = stbi_load(pathString.c_str(), &width, &height, nullptr, 4);
		
		gsl::finally([&] { stbi_image_free(memory); });
		
		UIImage image(gsl::narrow<uint32_t>(width), gsl::narrow<uint32_t>(height));
		
		uint64_t numBytes = width * height * 4;
		
		// ** Creates and copies data to the staging buffer **
		VmaAllocationInfo stagingBufferAllocInfo;
		VmaAllocation stagingBufferAlloc;
		VkBuffer stagingBuffer;
		
		const VmaAllocationCreateInfo stagingBufferAllocationCI =
		{
			VMA_ALLOCATION_CREATE_MAPPED_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY
		};
		
		VkBufferCreateInfo stagingBufferCreateInfo;
		InitBufferCreateInfo(stagingBufferCreateInfo, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, numBytes);
		
		CheckResult(vmaCreateBuffer(vulkan.allocator, &stagingBufferCreateInfo, &stagingBufferAllocationCI,
		                            &stagingBuffer, &stagingBufferAlloc, &stagingBufferAllocInfo));
		
		std::copy_n(memory, numBytes, reinterpret_cast<stbi_uc*>(stagingBufferAllocInfo.pMappedData));
		
		// ** Uploads data to the image **
		image.BeforeInitialTransfer(loadContext.GetCB());
		
		const VkBufferImageCopy copyRegion =
		{
			/* bufferOffset      */ 0,
			/* bufferRowLength   */ 0,
			/* bufferImageHeight */ 0,
			/* imageSubresource  */ { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
			/* imageOffset       */ { 0, 0, 0 },
			/* imageExtent       */ { image.m_width, image.m_height, 1 }
		};
		
		loadContext.GetCB().CopyBufferToImage(stagingBuffer, *image.m_image, copyRegion);
		
		image.AfterInitialTransfer(loadContext.GetCB());
		
		loadContext.TakeResource(VkHandle<VmaAllocation>(stagingBufferAlloc));
		loadContext.TakeResource(VkHandle<VkBuffer>(stagingBuffer));
		
		return image;
	}
	
	UIImage UIImage::CreateSingleColor(const glm::vec4& color, uint32_t width, uint32_t height, CommandBuffer& cb)
	{
		UIImage image(width, height);
		
		image.BeforeInitialTransfer(cb);
		
		const VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		cb.ClearColorImage(*image.m_image, reinterpret_cast<const VkClearColorValue*>(&color), subresourceRange);
		
		image.AfterInitialTransfer(cb);
		
		return image;
	}
	
	UIImage::UIImage(uint32_t width, uint32_t height)
	    : m_width(width), m_height(height), m_descriptorSet("UI_Image")
	{
		// ** Creates the image **
		VkImageCreateInfo imageCreateInfo;
		InitImageCreateInfo(imageCreateInfo, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, width, height);
		imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		
		const VmaAllocationCreateInfo allocationCI = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
		
		CheckResult(vmaCreateImage(vulkan.allocator, &imageCreateInfo, &allocationCI, m_image.GetCreateAddress(),
		                           m_allocation.GetCreateAddress(), nullptr));
		
		// ** Creates the image view **
		VkImageViewCreateInfo imageViewCreateInfo;
		InitImageViewCreateInfo(imageViewCreateInfo, *m_image, VK_IMAGE_VIEW_TYPE_2D_ARRAY,
		                        imageCreateInfo.format, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		
		CheckResult(vkCreateImageView(vulkan.device, &imageViewCreateInfo, nullptr, m_imageView.GetCreateAddress()));
		
		// ** Updates the descriptor set **
		const VkDescriptorImageInfo imageInfo = { VK_NULL_HANDLE, *m_imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		
		VkWriteDescriptorSet writeDS;
		m_descriptorSet.InitWriteDescriptorSet(writeDS, 0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, imageInfo);
		UpdateDescriptorSets(SingleElementSpan(writeDS));
	}
	
	void UIImage::BeforeInitialTransfer(CommandBuffer& cb)
	{
		VkImageMemoryBarrier imageBarrier;
		InitImageMemoryBarrier(imageBarrier, *m_image);
		imageBarrier.srcAccessMask = 0;
		imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		
		cb.PipelineBarrier(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		                   0, { }, { }, SingleElementSpan(imageBarrier));
	}
	
	void UIImage::AfterInitialTransfer(CommandBuffer& cb)
	{
		VkImageMemoryBarrier imageBarrier;
		InitImageMemoryBarrier(imageBarrier, *m_image);
		imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		
		cb.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		                   0, { }, { }, SingleElementSpan(imageBarrier));
	}
}
