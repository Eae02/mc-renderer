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
		
		void FillRenderList(class ChunkRenderList& renderList, class ChunkRenderList& shadowRenderList,
		                    const class WorldManager& worldManager, const class Camera& camera,
		                    const class Frustum& frustum, const class DirectionalShadowVolume& shadowVolume);
		
		ChunkVisibilityGraph GetVisibilityGraph(CommandBuffer& commandBuffer) const;
		
	private:
		struct FillQueueEntry
		{
			int m_lx;
			int m_lz;
			uint8_t m_y;
			uint8_t m_enterFace;
		};
		
		void Prepare(const class WorldManager& worldManager);
		
		template <typename GetWalkDirectionCallback, typename BoundingVolumeType>
		void ProcessFillQueue(class ChunkRenderList& renderList, const class WorldManager& worldManager,
		                      FillQueueEntry*& fillQueueBack, const BoundingVolumeType& boundingVolume,
		                      GetWalkDirectionCallback walkDirectionCallback);
		
		inline bool& GetVisited(int lx, int y, int lz)
		{
			return m_chunkVisited[(lz + y * m_lastRegionTableSize) * m_lastRegionTableSize + lx];
		}
		
		RegionCoordinate m_centerRegion;
		int m_lastRegionTableSize = 0;
		
		float m_maxDistToVisibleChunk;
		
		std::unique_ptr<FillQueueEntry[]> m_fillQueue;
		size_t m_fillQueueSize;
		
		//For each chunk in the loaded grid, stores whether it has been visited or not.
		std::unique_ptr<bool[]> m_chunkVisited;
	};
}
