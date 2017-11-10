#include "bufferpool.h"
#include "vkutils.h"

namespace MCR
{
	BufferPool::BufferPool(size_t bytesPerElement, size_t numElements, VkBufferUsageFlags bufferUsage)
	    : m_allocationTracker(numElements)
	{
		const VmaAllocationCreateInfo allocationCI = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
		
		VkBufferCreateInfo vertexBufferCreateInfo; // NOLINT
		InitBufferCreateInfo(vertexBufferCreateInfo, bufferUsage, numElements * bytesPerElement);
		
		CheckResult(vmaCreateBuffer(vulkan.allocator, &vertexBufferCreateInfo, &allocationCI,
		                            m_buffer.GetCreateAddress(), m_allocation.GetCreateAddress(), nullptr));
	}
	
	MCR::BufferPool::Allocation MCR::BufferPool::Allocate(uint64_t elementCount)
	{
		auto findResult = m_allocationTracker.FindAvailable(elementCount);
		if (!findResult.Found())
		{
			return { 0, 0 };
		}
		
		m_allocationTracker.Allocate(findResult, elementCount);
		
		return { findResult.GetFirstElement(), elementCount };
	}
}
