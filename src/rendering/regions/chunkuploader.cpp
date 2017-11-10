#include "chunkuploader.h"
#include "meshbuilder.h"

#include <vector>

namespace MCR
{
	ChunkUploader::HostBuffer::HostBuffer(uint64_t size) : m_size(size), m_fence(CreateVkFence())
	{
		VmaAllocationCreateInfo allocationCI = { };
		allocationCI.flags = VMA_ALLOCATION_CREATE_PERSISTENT_MAP_BIT;
		allocationCI.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		
		VmaAllocationInfo allocationInfo;
		
		VkBufferCreateInfo bufferCreateInfo;
		InitBufferCreateInfo(bufferCreateInfo, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size);
		CheckResult(vmaCreateBuffer(vulkan.allocator, &bufferCreateInfo, &allocationCI,
		                            m_buffer.GetCreateAddress(), m_allocation.GetCreateAddress(), &allocationInfo));
		
		m_memory = allocationInfo.pMappedData;
	}
	
	ChunkUploader::ChunkUploader()
	    : m_commandPool(CreateCommandPool(QUEUE_FAMILY_TRANSFER, 0))
	{
		
	}
	
	void ChunkUploader::BeginUploading(int64_t x, int64_t y, int64_t z, Region::ChunkConnectivity connectivity,
	                                   const MeshBuilder& meshBuilder)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		
		ChunkMesh chunk(meshBuilder.GetNumIndices(), meshBuilder.GetNumVertices(), connectivity);
		
		HostBuffer hostBuffer = AllocateHostBuffer(meshBuilder.GetRequiredBufferSize());
		
		meshBuilder.FillUploadBuffer(hostBuffer.m_memory);
		
		CommandBuffer commandBuffer(*m_commandPool);
		
		commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		
		chunk.Upload(commandBuffer, *hostBuffer.m_buffer);
		
		commandBuffer.End();
		
		VkSubmitInfo transferSubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		transferSubmitInfo.commandBufferCount = 1;
		transferSubmitInfo.pCommandBuffers = &commandBuffer.GetVkCB();
		vulkan.queues[QUEUE_FAMILY_TRANSFER]->Submit(1, &transferSubmitInfo, *hostBuffer.m_fence);
		
		m_tasks.push_back({ x, y, z, std::move(chunk), std::move(hostBuffer), std::move(commandBuffer) });
	}
	
	void ChunkUploader::WaitIdle()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		
		if (m_tasks.empty())
			return;
		
		VkFence* fences = reinterpret_cast<VkFence*>(alloca(m_tasks.size() * sizeof(VkFence)));
		std::transform(MAKE_RANGE(m_tasks), fences, [&] (const Task& task) { return *task.m_hostBuffer.m_fence; });
		
		WaitForFences({ fences, gsl::narrow<int>(m_tasks.size()) });
	}
	
	ChunkUploader::HostBuffer ChunkUploader::AllocateHostBuffer(uint64_t size)
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
