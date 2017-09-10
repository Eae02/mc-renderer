#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <vector>
#include <cstdint>

#include "../rendering/regions/regionuploader.h"
#include "../rendering/regions/regionmesh.h"
#include "regionbuildthread.h"
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
		
		inline const Camera& GetCamera() const
		{
			return m_camera;
		}
		
	private:
		void FillRenderListR(class RegionRenderList& renderList, const class Frustum& frustum,
		                     int minX, int minZ, int spanX, int spanZ) const;
		
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
			UpToDate,
			OutOfDate,
			Uploading
		};
		
		struct RegionEntry
		{
			RegionStates m_state;
			Region m_region;
			RegionMesh m_mesh;
			RegionMesh::Data m_newMeshData;
		};
		
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
		RegionBuildThread m_regionBuildThread;
		
		std::unique_ptr<World> m_world;
		Camera m_camera;
	};
}
