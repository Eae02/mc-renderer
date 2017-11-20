#include "causticstexture.h"
#include "shaders/shadermodules.h"
#include "../vulkan/instance.h"
#include "../loadcontext.h"

#include <fstream>
#include <zlib.h>

namespace MCR
{
	static constexpr uint32_t resolution = 512;
	static constexpr uint32_t depth = 128;
	static constexpr VkFormat format = VK_FORMAT_R8_UNORM;
	static constexpr float scale = 16;
	
	VkHandle<VkPipelineLayout> CausticsTexture::s_genPipelineLayout;
	VkHandle<VkPipeline> CausticsTexture::s_genPipeline;
	uint32_t CausticsTexture::s_workGroupSize;
	
	CausticsTexture::CausticsTexture()
	    : m_generateDescriptorSet("CausticsGen")
	{
		// ** Creates the image **
		VkImageCreateInfo imageCreateInfo;
		InitImageCreateInfo(imageCreateInfo, VK_IMAGE_TYPE_3D, format, resolution, resolution, depth);
		imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		
		VmaAllocationCreateInfo allocationCreateInfo = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
		
		CheckResult(vmaCreateImage(vulkan.allocator, &imageCreateInfo, &allocationCreateInfo,
		                           m_image.GetCreateAddress(), m_allocation.GetCreateAddress(), nullptr));
		
		// ** Creates the image view **
		VkImageViewCreateInfo viewCreateInfo;
		InitImageViewCreateInfo(viewCreateInfo, *m_image, VK_IMAGE_VIEW_TYPE_3D, format,
		                        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		CheckResult(vkCreateImageView(vulkan.device, &viewCreateInfo, nullptr, m_imageView.GetCreateAddress()));
		
		// ** Writes the image to the descriptor set **
		VkWriteDescriptorSet dsWrite;
		const VkDescriptorImageInfo imageInfo = { VK_NULL_HANDLE, *m_imageView, VK_IMAGE_LAYOUT_GENERAL };
		m_generateDescriptorSet.InitWriteDescriptorSet(dsWrite, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, imageInfo);
		UpdateDescriptorSets(SingleElementSpan(dsWrite));
	}
	
	struct SpecializationData
	{
		uint32_t m_workGroupSize[3];
		float m_positionScaleXY;
		float m_positionScaleZ;
	};
	
	void CausticsTexture::CreatePipelines()
	{
		s_workGroupSize = static_cast<uint32_t>(std::pow(vulkan.limits.maxComputeWorkGroupInvocations, 1.0 / 3.0));
		for (uint32_t maxWorkGroupSizeDim : vulkan.limits.maxComputeWorkGroupSize)
		{
			if (s_workGroupSize > maxWorkGroupSizeDim)
			{
				s_workGroupSize = maxWorkGroupSizeDim;
			}
		}
		
		//Rounds the work group size down to the previous power of two.
		s_workGroupSize = 1u << static_cast<uint32_t>(std::log2(s_workGroupSize));
		
		//Caps the work group size if is is larger than the larger of resolution and depth. 
		s_workGroupSize = std::min(std::max(resolution, depth), s_workGroupSize);
		
		Log("Caustics work group size: ", s_workGroupSize);
		
		// ** Creates the pipeline layout **
		
		VkDescriptorSetLayout setLayout = GetDescriptorSetLayout("CausticsGen");
		
		const VkPipelineLayoutCreateInfo layoutCreateInfo = 
		{
			/* sType                  */ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			/* pNext                  */ nullptr,
			/* flags                  */ 0,
			/* setLayoutCount         */ 1,
			/* pSetLayouts            */ &setLayout,
			/* pushConstantRangeCount */ 0,
			/* pPushConstantRanges    */ nullptr
		};
		
		VkPipelineLayout pipelineLayout;
		CheckResult(vkCreatePipelineLayout(vulkan.device, &layoutCreateInfo, nullptr, &pipelineLayout));
		s_genPipelineLayout = pipelineLayout;
		
		// ** Creates the pipeline **
		
		SpecializationData specializationData;
		std::fill(MAKE_RANGE(specializationData.m_workGroupSize), s_workGroupSize);
		specializationData.m_positionScaleXY = scale / static_cast<float>(resolution);
		specializationData.m_positionScaleZ = scale / static_cast<float>(depth);
		
		const VkSpecializationMapEntry specializationMapEntries[] = 
		{
			{ 0, offsetof(SpecializationData, m_workGroupSize), sizeof(SpecializationData::m_workGroupSize) },
			{ 1, offsetof(SpecializationData, m_positionScaleXY), sizeof(SpecializationData::m_positionScaleXY) },
			{ 2, offsetof(SpecializationData, m_positionScaleZ), sizeof(SpecializationData::m_positionScaleZ) }
		};
		
		const VkSpecializationInfo specializationInfo = 
		{
			/* mapEntryCount */ static_cast<uint32_t>(ArrayLength(specializationMapEntries)),
			/* pMapEntries   */ specializationMapEntries,
			/* dataSize      */ sizeof(SpecializationData),
			/* pData         */ &specializationData
		};
		
		VkComputePipelineCreateInfo pipelineCreateInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
		pipelineCreateInfo.stage = 
		{
			/* sType               */ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			/* pNext               */ nullptr,
			/* flags               */ 0,
			/* stage               */ VK_SHADER_STAGE_COMPUTE_BIT,
			/* module              */ GetShaderModule("caustics.cs"),
			/* pName               */ "main",
			/* pSpecializationInfo */ &specializationInfo
		};
		pipelineCreateInfo.layout = pipelineLayout;
		pipelineCreateInfo.basePipelineIndex = -1;
		
		CheckResult(vkCreateComputePipelines(vulkan.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
		                                     s_genPipeline.GetCreateAddress()));
	}
	
	void CausticsTexture::Render(CommandBuffer& commandBuffer)
	{
		// ** Changes the image layout to GENERAL **
		VkImageMemoryBarrier barrier;
		InitImageMemoryBarrier(barrier, *m_image);
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		
		commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
		                              { }, { }, SingleElementSpan(barrier));
		
		// ** Generates caustics **
		
		commandBuffer.BindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, *s_genPipeline);
		
		const VkDescriptorSet descriptorSets[] = { *m_generateDescriptorSet };
		commandBuffer.BindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, *s_genPipelineLayout, 0, descriptorSets);
		
		const uint32_t dispatchXY = DivRoundUp(resolution, s_workGroupSize);
		commandBuffer.Dispatch(dispatchXY, dispatchXY, DivRoundUp(depth, s_workGroupSize));
		
		// ** Changes the image layout to SHADER_READ_ONLY_OPTIMAL **
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		
		commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		                              { }, { }, SingleElementSpan(barrier));
	}
	
	static constexpr uint64_t stagingBufferSize = resolution * resolution * depth;
	
	static const VkBufferImageCopy bufferImageCopy =
	{
		/* bufferOffset      */ 0,
		/* bufferRowLength   */ 0,
		/* bufferImageHeight */ 0,
		/* imageSubresource  */ { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
		/* imageOffset       */ { 0, 0, 0 },
		/* imageExtent       */ { resolution, resolution, depth }
	};
	
	bool CausticsTexture::TryLoad(const fs::path& path, LoadContext& loadContext)
	{
		std::ifstream stream(path, std::ios::binary);
		if (!stream)
			return false;
		
		uint32_t cacheResolution;
		stream.read(reinterpret_cast<char*>(&cacheResolution), sizeof(uint32_t));
		
		uint32_t cacheDepth;
		stream.read(reinterpret_cast<char*>(&cacheDepth), sizeof(uint32_t));
		
		if (cacheResolution != resolution || cacheDepth != depth)
			return false;
		
		//Allocates a staging buffer
		VkBuffer stagingBuffer;
		VmaAllocation stagingBufferAllocation;
		void* stagingBufferMem;
		CreateStagingBuffer(stagingBufferSize, &stagingBuffer, &stagingBufferAllocation, &stagingBufferMem);
		
		z_stream inflateStream = { };
		inflateInit(&inflateStream);
		
		char inBuffer[256];
		
		inflateStream.next_out = reinterpret_cast<Bytef*>(stagingBufferMem);
		inflateStream.avail_out = static_cast<uInt>(stagingBufferSize);
		
		// ** Inflates the data 256 bytes at a time **
		int status;
		do
		{
			stream.read(inBuffer, sizeof(inBuffer));
			
			inflateStream.avail_in = static_cast<uInt>(stream.gcount());
			inflateStream.next_in = reinterpret_cast<Bytef*>(inBuffer);
			
			if (inflateStream.avail_in == 0)
				break;
			
			status = inflate(&inflateStream, Z_NO_FLUSH);
			
			if (status == Z_MEM_ERROR)
				throw std::bad_alloc();
			if (status == Z_STREAM_ERROR || status == Z_DATA_ERROR || status == Z_NEED_DICT)
				throw std::runtime_error("Invalid deflate stream.");
		} while(status != Z_STREAM_END);
		
		inflateEnd(&inflateStream);
		
		// ** Changes the image layout to TRANSFER_DST_OPTIMAL **
		VkImageMemoryBarrier barrier;
		InitImageMemoryBarrier(barrier, *m_image);
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		
		loadContext.GetCB().PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
		                                    { }, { }, SingleElementSpan(barrier));
		
		// ** Uploads image data **
		loadContext.GetCB().CopyBufferToImage(stagingBuffer, *m_image, bufferImageCopy);
		
		// ** Changes the image layout to SHADER_READ_ONLY_OPTIMAL **
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		
		loadContext.GetCB().PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		                                    { }, { }, SingleElementSpan(barrier));
		
		loadContext.TakeResource(VkHandle<VkBuffer>(stagingBuffer));
		loadContext.TakeResource(VkHandle<VmaAllocation>(stagingBufferAllocation));
		
		return true;
	}
	
	void CausticsTexture::Save(const fs::path& path) const
	{
		//Allocates a download buffer
		const VmaAllocationCreateInfo allocationCI =
		{
			VMA_ALLOCATION_CREATE_PERSISTENT_MAP_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY
		};
		
		VkBufferCreateInfo downloadBufferCI;
		InitBufferCreateInfo(downloadBufferCI, VK_BUFFER_USAGE_TRANSFER_DST_BIT, stagingBufferSize);
		
		VkBuffer downloadBuffer;
		VmaAllocation downloadBufferAllocation;
		VmaAllocationInfo downloadBufferAllocationInfo;
		CheckResult(vmaCreateBuffer(vulkan.allocator, &downloadBufferCI, &allocationCI,
		                            &downloadBuffer, &downloadBufferAllocation, &downloadBufferAllocationInfo));
		
		CommandBuffer commandBuffer(vulkan.stdCommandPools[QUEUE_FAMILY_GRAPHICS]);
		
		commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		
		// ** Changes the image layout to TRANSFER_SRC_OPTIMAL **
		VkImageMemoryBarrier barrier;
		InitImageMemoryBarrier(barrier, *m_image);
		barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		
		commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
		                              { }, { }, SingleElementSpan(barrier));
		
		// ** Copies the image contents to the download buffer **
		commandBuffer.CopyImageToBuffer(*m_image, downloadBuffer, bufferImageCopy);
		
		commandBuffer.End();
		
		VkHandle<VkFence> fence = CreateVkFence();
		
		VkSubmitInfo submit = 
		{
			/* sType                */ VK_STRUCTURE_TYPE_SUBMIT_INFO,
			/* pNext                */ nullptr,
			/* waitSemaphoreCount   */ 0,
			/* pWaitSemaphores      */ nullptr,
			/* pWaitDstStageMask    */ nullptr,
			/* commandBufferCount   */ 1,
			/* pCommandBuffers      */ &commandBuffer.GetVkCB(),
			/* signalSemaphoreCount */ 0,
			/* pSignalSemaphores    */ nullptr
		};
		vulkan.queues[QUEUE_FAMILY_TRANSFER]->Submit(1, &submit, *fence);
		
		WaitForFence(*fence, UINT64_MAX);
		
		std::ofstream stream(path, std::ios::binary);
		
		stream.write(reinterpret_cast<const char*>(&resolution), sizeof(uint32_t));
		stream.write(reinterpret_cast<const char*>(&depth), sizeof(uint32_t));
		
		char outBuffer[256];
		
		z_stream deflateStream = { };
		
		deflateStream.avail_in = stagingBufferSize;
		deflateStream.next_in = reinterpret_cast<const Bytef*>(downloadBufferAllocationInfo.pMappedData);
		
		if (deflateInit(&deflateStream, Z_DEFAULT_COMPRESSION) != Z_OK)
		{
			throw std::runtime_error("Error initializing ZLIB.");
		}
		
		// ** Deflates and writes the data 256 bytes at a time **
		int status;
		do
		{
			deflateStream.avail_out = sizeof(outBuffer);
			deflateStream.next_out = reinterpret_cast<Bytef*>(outBuffer);
			
			status = deflate(&deflateStream, Z_FINISH);
			assert(status != Z_STREAM_ERROR);
			
			std::streamsize bytesCompressed = sizeof(outBuffer) - deflateStream.avail_out;
			stream.write(outBuffer, bytesCompressed);
		} while (deflateStream.avail_out == 0);
		
		deflateEnd(&deflateStream);
	}
}
