#include "chunkvisibilitycalculator.h"
#include "chunkrenderlist.h"
#include "frustum.h"
#include "../blocks/sides.h"
#include "../world/worldmanager.h"

namespace MCR
{
	
	ChunkVisibilityCalculator::ChunkVisibilityCalculator()
	{
		
	}
	
	void ChunkVisibilityCalculator::Prepare(const WorldManager& worldManager)
	{
		size_t maxChunks = worldManager.m_regionTableSize * worldManager.m_regionTableSize * Region::ChunkCount;
		
		if (m_lastRegionTableSize != worldManager.m_regionTableSize)
		{
			m_fillQueue = std::make_unique<FillQueueEntry[]>(maxChunks);
			
			m_chunkVisited = std::make_unique<bool[]>(maxChunks);
			
			m_lastRegionTableSize = worldManager.m_regionTableSize;
		}
		
		m_centerRegion.x = worldManager.m_centerRegionX;
		m_centerRegion.z = worldManager.m_centerRegionZ;
		
		std::fill_n(m_chunkVisited.get(), maxChunks, false);
	}
	
	inline bool ChunkIntersectsFrustum(const Frustum& frustum, int64_t x, int y, int64_t z)
	{
		const glm::vec3 basePos(x * Region::Size, y * Region::Size, z * Region::Size);
		
		const AABoundingBox boundingBox(basePos, basePos + glm::vec3(Region::Size));
		
		return frustum.Intersects(boundingBox);
	}
	
	void ChunkVisibilityCalculator::FillRenderList(ChunkRenderList& renderList, const WorldManager& worldManager,
	                                               const Camera& camera, const Frustum& frustum)
	{
		const int64_t cameraChunkX = std::floor(camera.GetPosition().x / Region::Size);
		const int     cameraChunkY = std::floor(camera.GetPosition().y / Region::Size);
		const int64_t cameraChunkZ = std::floor(camera.GetPosition().z / Region::Size);
		
		ChunkMesh* currentChunkMesh = worldManager.GetChunkMeshForRendering(cameraChunkX, cameraChunkY, cameraChunkZ);
		if (currentChunkMesh == nullptr)
			return;
		
		Prepare(worldManager);
		
		//Adds the current chunk to the render list and marks it visited.
		if (currentChunkMesh->HasData())
		{
			renderList.Add(*currentChunkMesh);
		}
		GetVisited(worldManager.m_loadDistance, cameraChunkY, worldManager.m_loadDistance) = true;
		
		FillQueueEntry* fillQueueFront = m_fillQueue.get();
		FillQueueEntry* fillQueueBack = m_fillQueue.get();
		
		//Adds the current chunk's neighbors to the traversal queue.
		for (uint8_t n = 0; n < 6; n++)
		{
			const int64_t nx = cameraChunkX + BlockNormals[n].x;
			const int     ny = cameraChunkY + BlockNormals[n].y;
			const int64_t nz = cameraChunkZ + BlockNormals[n].z;
			
			if (ny < 0 || ny >= Region::ChunkCount || !ChunkIntersectsFrustum(frustum, nx, ny, nz))
				continue;
			
			const int nlx = worldManager.m_loadDistance + BlockNormals[n].x;
			const int nlz = worldManager.m_loadDistance + BlockNormals[n].z;
			GetVisited(nlx, ny, nlz) = true;
			
			*(fillQueueBack++) = { nlx, nlz, static_cast<uint8_t>(ny), OpposingSides[n] };
		}
		
		while (fillQueueBack != fillQueueFront)
		{
			//Dequeues an item from the traversal queue.
			const FillQueueEntry& stackEntry = *(fillQueueFront++);
			
			const RegionCoordinate regionCoord = worldManager.GetWorldRegionCoord(stackEntry.m_lx, stackEntry.m_lz);
			
			ChunkMesh* mesh = worldManager.GetChunkMeshForRendering(regionCoord.x, stackEntry.m_y, regionCoord.z);
			
			if (mesh == nullptr)
				continue;
			
			if (mesh->HasData())
			{
				renderList.Add(*mesh);
			}
			
			if (worldManager.GetRegion(regionCoord)->IsChunkOpaque(stackEntry.m_y))
				continue;
			
			glm::vec3 centerChunk((static_cast<float>(regionCoord.x) + 0.5f) * Region::Size,
			                      (static_cast<float>(stackEntry.m_y) + 0.5f) * Region::Size,
			                      (static_cast<float>(regionCoord.z) + 0.5f) * Region::Size);
			
			for (uint8_t n = 0; n < 6; n++)
			{
				if (n == stackEntry.m_enterFace || 
				    (mesh->HasData() && !mesh->IsSideConnected(stackEntry.m_enterFace, n)))
				{
					continue;
				}
				
				const int nlx = stackEntry.m_lx + BlockNormals[n].x;
				const int ny  = static_cast<int>(stackEntry.m_y) + BlockNormals[n].y;
				const int nlz = stackEntry.m_lz + BlockNormals[n].z;
				
				if (nlx < 0 || ny < 0 || nlz < 0 || nlx >= m_lastRegionTableSize ||
				    ny >= Region::ChunkCount || nlz >= m_lastRegionTableSize)
				{
					continue;
				}
				
				bool& visited = GetVisited(nlx, ny, nlz);
				if (visited)
					continue;
				
				glm::vec3 edgeToCamera = camera.GetPosition() - (centerChunk + glm::vec3(BlockNormals[n] * (Region::Size / 2)));
				
				if (glm::dot(edgeToCamera, glm::vec3(BlockNormals[n])) > 0)
					continue;
				
				if (!ChunkIntersectsFrustum(frustum, regionCoord.x + BlockNormals[n].x, ny,
				                            regionCoord.z + BlockNormals[n].z))
				{
					continue;
				}
				
				visited = true;
				*(fillQueueBack++) = { nlx, nlz, static_cast<uint8_t>(ny), OpposingSides[n] };
			}
		}
	}
	
	ChunkVisibilityGraph ChunkVisibilityCalculator::GetVisibilityGraph(CommandBuffer& commandBuffer) const
	{
		return ChunkVisibilityGraph(m_chunkVisited.get(), m_lastRegionTableSize, m_centerRegion, commandBuffer);
	}
}
