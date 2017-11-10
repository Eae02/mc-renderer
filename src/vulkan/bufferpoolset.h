#pragma once

#include "bufferpool.h"

namespace MCR
{
	class BufferPoolSet
	{
	public:
		struct Allocation : BufferPool::Allocation
		{
			VkBuffer m_buffer;
		};
		
		inline BufferPoolSet(size_t bytesPerElement, size_t numElements, VkBufferUsageFlags bufferUsage)
		    : m_bytesPerElement(bytesPerElement), m_numElements(numElements), m_bufferUsage(bufferUsage) { }
		
		Allocation Allocate(uint64_t elementCount);
		
		void Free(const Allocation& allocation);
		
	private:
		std::vector<BufferPool> m_pools;
		
		size_t m_bytesPerElement;
		size_t m_numElements;
		VkBufferUsageFlags m_bufferUsage;
	};
}
