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

namespace MCR
{
	class WorldManager
	{
	public:
		WorldManager();
		
		void SaveAll(bool wait);
		
		void SetRenderDistance(int renderDist);
		
		void Update(float dt, const class InputState& inputState);
		
		inline void FillRenderList(class RegionRenderList& renderList, const class Frustum& frustum) const
		{
			FillRenderListR(renderList, frustum, 0, 0, m_regionTableSize, m_regionTableSize);
		}
		
		void SetWorld(std::unique_ptr<World> world);
		
		Region* GetRegion(RegionCoordinate coordinate);
		
		void MarkOutOfDate(RegionCoordinate coordinate, uint32_t chunkY);
		
		inline const Camera& GetCamera() const
		{
			return m_camera;
		}
		
	private:
		void FillRenderListR(class RegionRenderList& renderList, const class Frustum& frustum,
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
		
		MeshBuilder m_meshBuilder;
		ChunkBuildThread m_chunkBuildThread;
		
		std::unique_ptr<World> m_world;
		Camera m_camera;
	};
}
