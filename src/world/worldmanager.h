#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <vector>
#include <cstdint>
#include <bitset>

#include "chunkbuildthread.h"
#include "regiongeneratethread.h"
#include "regioniothread.h"
#include "world.h"
#include "camera.h"
#include "../rendering/regions/watermesh.h"
#include "../rendering/regions/watermeshbuilder.h"

namespace MCR
{
	class WorldManager
	{
		friend class ChunkVisibilityCalculator;
		
	public:
		WorldManager();
		
		void WaitIdle();
		
		void SaveAll();
		
		void SetRenderDistance(int renderDist);
		
		void Update(float dt, const class InputState& inputState);
		
		void BuildWater(CommandBuffer& commandBuffer);
		
		inline void FillRenderList(class ChunkRenderList& renderList, const class Frustum& frustum) const
		{
			FillRenderListR(renderList, frustum, 0, 0, m_regionTableSize, m_regionTableSize);
		}
		
		struct MeshRenderInfo
		{
			ChunkMesh* m_chunkMesh;
			WaterMesh* m_waterMesh;
		};
		
		MeshRenderInfo GetChunkMeshRenderInfo(int64_t x, int y, int64_t z) const;
		
		void SetWorld(std::unique_ptr<World> world);
		
		Region* GetRegion(RegionCoordinate coordinate);
		
		const Region* GetRegion(RegionCoordinate coordinate) const
		{
			return const_cast<WorldManager*>(this)->GetRegion(coordinate);
		}
		
		void MarkOutOfDate(RegionCoordinate coordinate, uint32_t chunkY);
		
		inline const Camera& GetCamera() const
		{
			return m_camera;
		}
		
		bool IsCameraUnderWater(float& waterPlaneY) const;
		
	private:
		void FillRenderListR(class ChunkRenderList& renderList, const class Frustum& frustum,
		                     int minX, int minZ, int spanX, int spanZ) const;
		
		inline RegionCoordinate GetWorldRegionCoord(int x, int z) const
		{
			return {
				static_cast<int64_t>(x - m_loadDistance) + m_centerRegionX,
				static_cast<int64_t>(z - m_loadDistance) + m_centerRegionZ
			};
		}
		
		inline int GetRegionIndex(int x, int z) const
		{
			if (x < 0 || z < 0 || x >= m_regionTableSize || z >= m_regionTableSize)
				return -1;
			return z + x * m_regionTableSize;
		}
		
		std::unique_ptr<RegionIOThread> m_ioThread;
		
		RegionGenerateThread m_generateThread;
		
		enum class RegionStates
		{
			Loading,
			LoadedNotBuilt,
			Building,
			Uploading,
			Built
		};
		
		struct RegionEntry
		{
			RegionStates m_state;
			std::shared_ptr<Region> m_region;
			std::bitset<Region::ChunkCount> m_meshesOutOfDate;
			std::array<ChunkMesh, Region::ChunkCount> m_meshes; //Performance improvement: don't allocate statically (faster move).
			std::array<WaterMesh, Region::ChunkCount> m_waterMeshes;
		};
		
		RegionEntry* RegionEntryFromGlobalCoordinate(RegionCoordinate coordinate);
		
		RegionEntry* AllocateRegionEntry();
		void FreeRegionEntry(RegionEntry* entry);
		
		int m_renderDistanceSq = 0;
		int m_loadDistance = 0;
		int m_regionTableSize = 0;
		
		int64_t m_centerRegionX = 0;
		int64_t m_centerRegionZ = 0;
		bool m_hasUpdated = false;
		
		std::unique_ptr<RegionEntry[]> m_regionsAllocation;
		std::vector<RegionEntry*> m_availableRegions;
		
		std::vector<RegionEntry*> m_regions[2];
		
		struct OutOfDateWaterMesh
		{
			RegionCoordinate m_coordinate;
			uint32_t m_chunkY;
		};
		std::vector<OutOfDateWaterMesh> m_outOfDateWaterList;
		
		MeshBuilder m_meshBuilder;
		WaterMeshBuilder m_waterMeshBuilder;
		ChunkBuildThread m_chunkBuildThread;
		
		std::unique_ptr<World> m_world;
		Camera m_camera;
	};
}
