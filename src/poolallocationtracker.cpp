#include "poolallocationtracker.h"

namespace MCR
{
	PoolAllocationTracker::PoolAllocationTracker(uint64_t elementCount)
	{
		m_availableBlocks.emplace_back(0, elementCount);
	}
	
	PoolAllocationTracker::FindAvailableResult PoolAllocationTracker::FindAvailable(uint64_t elementCount)
	{
		ssize_t blockIndex = -1;
		
		for (size_t i = 0; i < m_availableBlocks.size(); i++)
		{
			if (m_availableBlocks[i].m_elementCount >= elementCount)
			{
				if (blockIndex != -1 && m_availableBlocks[blockIndex].m_elementCount < m_availableBlocks[i].m_elementCount)
					continue;
				blockIndex = i;
				
				if (m_availableBlocks[i].m_elementCount == elementCount)
					break;
			}
		}
		
		if (blockIndex == -1)
			return { };
		
		return PoolAllocationTracker::FindAvailableResult(m_availableBlocks[blockIndex]);
	}
	
	void PoolAllocationTracker::Allocate(const PoolAllocationTracker::FindAvailableResult& availableResult,
	                                     uint64_t elementCount)
	{
		if (availableResult.m_block->m_elementCount == elementCount)
		{
			*availableResult.m_block = m_availableBlocks.back();
			m_availableBlocks.pop_back();
		}
		else
		{
			availableResult.m_block->m_firstElement += elementCount;
			availableResult.m_block->m_elementCount -= elementCount;
		}
	}
	
	void PoolAllocationTracker::Free(uint64_t firstElement, uint64_t elementCount)
	{
		ssize_t prevBlockIndex = -1;
		ssize_t nextBlockIndex = -1;
		
		const uint64_t nextBlockFirstElement = firstElement + elementCount;
		
		for (size_t i = 0; i < m_availableBlocks.size(); i++)
		{
			if (m_availableBlocks[i].m_firstElement == nextBlockFirstElement)
			{
				nextBlockIndex = i;
				if (prevBlockIndex != -1)
					break; //Both previous and next block have been found.
			}
			
			if (m_availableBlocks[i].m_firstElement + m_availableBlocks[i].m_elementCount == firstElement)
			{
				prevBlockIndex = i;
				if (nextBlockIndex != -1)
					break; //Both previous and next block have been found.
			}
		}
		
		if (prevBlockIndex == -1 && nextBlockIndex == -1)
		{
			//Neither a next or previous block exist.
			m_availableBlocks.emplace_back(firstElement, elementCount);
		}
		else if (prevBlockIndex != -1 && nextBlockIndex != -1)
		{
			//Both a next and previous block exist.
			
			//Increases the span of the previous block to cover the freed block and the next block.
			m_availableBlocks[prevBlockIndex].m_elementCount +=
			        elementCount + m_availableBlocks[nextBlockIndex].m_elementCount;
			
			//Removes the next block.
			m_availableBlocks[nextBlockIndex] = m_availableBlocks.back();
			m_availableBlocks.pop_back();
		}
		else if (prevBlockIndex != -1)
		{
			//Only a previous block exists.
			
			//Increases the span of the previous block to cover the freed block.
			m_availableBlocks[prevBlockIndex].m_elementCount += elementCount;
		}
		else
		{
			//Only a next block exists.
			
			//Increases the span of the next block and moves it back so it also covers the freed block.
			m_availableBlocks[nextBlockIndex].m_firstElement -= elementCount;
			m_availableBlocks[nextBlockIndex].m_elementCount += elementCount;
		}
	}
}
