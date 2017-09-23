#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace MCR
{
	class PoolAllocationTracker
	{
	private:
		struct AvailableBlock
		{
			uint64_t m_firstElement;
			uint64_t m_elementCount;
			
			inline AvailableBlock(uint64_t firstElement, uint64_t elementCount)
			    : m_firstElement(firstElement), m_elementCount(elementCount) { }
		};
		
	public:
		class FindAvailableResult
		{
			friend class PoolAllocationTracker;
			
		public:
			inline FindAvailableResult()
			    : m_firstElement(0), m_block(nullptr) { }
			
			inline bool Found() const
			{
				return m_block != nullptr;
			}
			
			inline uint64_t GetFirstElement() const
			{
				return m_firstElement;
			}
			
		private:
			inline FindAvailableResult(AvailableBlock& block)
			    : m_firstElement(block.m_firstElement), m_block(&block) { }
			
			uint64_t m_firstElement;
			AvailableBlock* m_block;
		};
		
		explicit PoolAllocationTracker(uint64_t elementCount);
		
		//Locates an available range of elements. Does not mark the range as allocated!.
		FindAvailableResult FindAvailable(uint64_t elementCount);
		
		//Marks a range of elements as allocated.
		void Allocate(const FindAvailableResult& availableResult, uint64_t elementCount);
		
		//Marks a range of elements as available.
		void Free(uint64_t firstElement, uint64_t elementCount);
		
	private:
		std::vector<AvailableBlock> m_availableBlocks;
	};
}
