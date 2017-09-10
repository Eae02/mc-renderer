#include "regionuploader.h"

namespace MCR
{
	RegionUploader::HostBuffer::HostBuffer(uint64_t size) : m_size(size), m_fence(CreateVkFence())
	{
		VmaMemoryRequirements memoryRequirements = { };
		memoryRequirements.flags = VMA_MEMORY_REQUIREMENT_OWN_MEMORY_BIT | VMA_MEMORY_REQUIREMENT_PERSISTENT_MAP_BIT;
		memoryRequirements.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		
		VmaAllocationInfo allocationInfo;
		
		VkBufferCreateInfo bufferCreateInfo;
		InitBufferCreateInfo(bufferCreateInfo, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size);
		CheckResult(vmaCreateBuffer(vulkan.allocator, &bufferCreateInfo, &memoryRequirements,
		                            m_buffer.GetCreateAddress(), m_allocation.GetCreateAddress(), &allocationInfo));
		
		m_memory = allocationInfo.pMappedData;
	}
	
	RegionUploader::RegionUploader()
	    : m_commandPool(CreateCommandPool(QUEUE_FAMILY_TRANSFER, 0))
	{
		
	}
	
	void RegionUploader::BeginUploading(int64_t x, int64_t z, const MeshBuilder& meshBuilder)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		
		uint64_t size = meshBuilder.GetRequiredBufferSize();
		
		RegionDataBuffer deviceBuffer = RegionDataBuffer::Allocate(size);
		
		HostBuffer hostBuffer = AllocateHostBuffer(size);
		
		meshBuilder.FillUploadBuffer(hostBuffer.m_memory);
		
		CommandBuffer commandBuffer(*m_commandPool);
		commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		
		//Copies data from the host buffer to the device buffer.
		commandBuffer.CopyBuffer(*hostBuffer.m_buffer, deviceBuffer.GetBuffer(), { 0, 0, size });
		
		//Releases the device buffer from the transfer queue.
		VkBufferMemoryBarrier barrier = 
		{
			/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			/* pNext               */ nullptr,
			/* srcAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
			/* dstAccessMask       */ 0,
			/* srcQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_TRANSFER],
			/* dstQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_GRAPHICS],
			/* buffer              */ deviceBuffer.GetBuffer(),
			/* offset              */ 0,
			/* size                */ size
		};
		commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
		                              { }, SingleElementSpan(barrier), { });
		
		commandBuffer.End();
		
		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer.GetVkCB();
		vulkan.queues[QUEUE_FAMILY_TRANSFER]->Submit(1, &submitInfo, *hostBuffer.m_fence);
		
		m_tasks.push_back({ x, z, std::move(deviceBuffer), std::move(hostBuffer), std::move(commandBuffer) });
	}
	
	RegionUploader::HostBuffer RegionUploader::AllocateHostBuffer(uint64_t size)
	{
		auto it = std::find_if(MAKE_RANGE(m_hostBuffers), [&] (const HostBuffer& buffer)
		{
			return buffer.m_size >= size && vkGetFenceStatus(vulkan.device, *buffer.m_fence) == VK_SUCCESS;
		});
		
		if (it != m_hostBuffers.end())
		{
			HostBuffer result = std::move(*it);
			CheckResult(vkResetFences(vulkan.device, 1, &*result.m_fence));
			
			if (std::next(it) != m_hostBuffers.end())
			{
				*it = std::move(m_hostBuffers.back());
			}
			m_hostBuffers.pop_back();
			
			return result;
		}
		
		return HostBuffer(RoundToNextMultiple<uint64_t>(size, 1024));
	}
}
