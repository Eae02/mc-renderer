#include "bufferpoolset.h"

namespace MCR
{
	BufferPoolSet::Allocation BufferPoolSet::Allocate(uint64_t elementCount)
	{
		BufferPool::Allocation allocation = { 0, 0 };
		VkBuffer buffer = VK_NULL_HANDLE;
		
		for (BufferPool& pool : m_pools)
		{
			allocation = pool.Allocate(elementCount);
			if (allocation.m_elementCount != 0)
			{
				buffer = pool.GetBuffer();
				break;
			}
		}
		
		if (allocation.m_elementCount == 0)
		{
			BufferPool& pool = m_pools.emplace_back(m_bytesPerElement, m_numElements, m_bufferUsage);
			allocation = pool.Allocate(elementCount);
			buffer = pool.GetBuffer();
		}
		
		Allocation result;
		result.m_buffer = buffer;
		result.m_elementCount = allocation.m_elementCount;
		result.m_firstElement = allocation.m_firstElement;
		return result;
	}
	
	void BufferPoolSet::Free(const BufferPoolSet::Allocation& allocation)
	{
		for (BufferPool& pool : m_pools)
		{
			if (allocation.m_buffer == pool.GetBuffer())
			{
				pool.Free(allocation);
				return;
			}
		}
		
		//Throw exception?
	}
}
