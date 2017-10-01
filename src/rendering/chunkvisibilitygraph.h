#pragma once

#include <cstddef>

#include "../world/region.h"

namespace MCR
{
	class ChunkVisibilityGraph
	{
	public:
		ChunkVisibilityGraph(const bool* visitedChunks, int regionTableSize, RegionCoordinate centerRegion,
		                     CommandBuffer& commandBuffer);
		
		void Draw(CommandBuffer& commandBuffer) const;
		
	private:
		VkHandle<VmaAllocation, VkHandleDestroyTime::Delayed> m_allocation;
		VkHandle<VkBuffer, VkHandleDestroyTime::Delayed> m_buffer;
		
		uint32_t m_numIndices;
	};
}
