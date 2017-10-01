#pragma once

#include <vector>

#include "chunkvisibilitygraph.h"
#include "../world/region.h"

namespace MCR
{
	class ChunkVisibilityCalculator
	{
	public:
		ChunkVisibilityCalculator();
		
		void FillRenderList(class ChunkRenderList& renderList, const class WorldManager& worldManager,
		                    const class Camera& camera, const class Frustum& frustum);
		
		ChunkVisibilityGraph GetVisibilityGraph(CommandBuffer& commandBuffer) const;
		
	private:
		void Prepare(const class WorldManager& worldManager);
		
		inline bool& GetVisited(int lx, int y, int lz)
		{
			return m_chunkVisited[(lz + y * m_lastRegionTableSize) * m_lastRegionTableSize + lx];
		}
		
		RegionCoordinate m_centerRegion;
		int m_lastRegionTableSize = 0;
		
		struct FillQueueEntry
		{
			int m_lx;
			int m_lz;
			uint8_t m_y;
			uint8_t m_enterFace;
		};
		
		std::unique_ptr<FillQueueEntry[]> m_fillQueue;
		
		//For each chunk in the loaded grid, stores whether it has been visited or not.
		std::unique_ptr<bool[]> m_chunkVisited;
	};
}
