#pragma once

#include "vkhandle.h"
#include "../poolallocationtracker.h"

namespace MCR
{
	class BufferPool
	{
	public:
		struct Allocation
		{
			uint64_t m_firstElement;
			uint64_t m_elementCount;
		};
		
		BufferPool(size_t bytesPerElement, size_t numElements, VkBufferUsageFlags bufferUsage);
		
		Allocation Allocate(uint64_t elementCount);
		
		inline void Free(const Allocation& allocation)
		{
			m_allocationTracker.Free(allocation.m_firstElement, allocation.m_elementCount);
		}
		
		inline VkBuffer GetBuffer() const
		{
			return *m_buffer;
		}
		
	private:
		VkHandle<VmaAllocation> m_allocation;
		VkHandle<VkBuffer> m_buffer;
		PoolAllocationTracker m_allocationTracker;
	};
}
