#include "chunkbuildthread.h"
#include "region.h"
#include "../rendering/regions/buildchunkmesh.h"

namespace MCR
{
	ChunkBuildThread::~ChunkBuildThread()
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_exit = true;
			m_signal.notify_one();
		}
		
		m_thread.join();
	}
	
	void ChunkBuildThread::BuildSync(const Region& region, uint32_t chunkY, gsl::span<const Region*> neighbors,
	                                 MeshBuilder& meshBuilder)
	{
		ChunkMeshBuildParams buildParams;
		buildParams.m_meshBuilder = &meshBuilder;
		buildParams.m_region = &region;
		buildParams.m_chunkY = chunkY;
		std::copy_n(neighbors.begin(), 4, buildParams.m_neighbors);
		
		meshBuilder.Reset();
		
		BuildChunkMesh(buildParams);
		
		if (!meshBuilder.Empty())
		{
			const Region::ChunkConnectivity connectivity = region.CalculateConnectivity(chunkY);
			
			m_uploader.BeginUploading(region.GetX(), chunkY, region.GetZ(), connectivity, meshBuilder);
		}
	}
	
	void ChunkBuildThread::ThreadTarget()
	{
		while (true)
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			
			m_signal.wait(lock, [&] { return !m_buildCommands.empty() || m_exit; });
			
			if (m_exit)
				break;
			
			//Selects the closest region to the camera for building.
			long selectedIndex = -1;
			uint64_t selectedChunkDistFromCameraSq = 0;
			for (size_t i = 0; i < m_buildCommands.size(); i++)
			{
				const int64_t dx = m_buildCommands[i].m_coordinate.x - m_cameraChunkX;
				const int64_t dy = static_cast<int64_t>(m_buildCommands[i].m_chunkY) - m_cameraChunkY;
				const int64_t dz = m_buildCommands[i].m_coordinate.z - m_cameraChunkZ;
				
				const uint64_t distFromCameraSq = dx * dx + dy * dy + dz * dz;
				if (selectedChunkDistFromCameraSq > distFromCameraSq || selectedIndex == -1)
				{
					selectedIndex = i;
					selectedChunkDistFromCameraSq = distFromCameraSq;
				}
			}
			
			BuildCommand buildCommand = std::move(m_buildCommands[selectedIndex]);
			
			//Removes the selected build command from the list.
			if (selectedIndex != static_cast<long>(m_buildCommands.size()) - 1)
			{
				m_buildCommands[selectedIndex] = std::move(m_buildCommands.back());
			}
			m_buildCommands.pop_back();
			
			lock.unlock();
			
			std::shared_ptr<const Region> region = buildCommand.m_region.lock();
			if (region == nullptr)
				continue;
			
			std::array<std::shared_ptr<const Region>, 4> neighborRegionsSP;
			std::array<const Region*, 4> neighborRegionsP;
			
			bool anyNeighborNull = false;
			for (int i = 0; i < 4; i++)
			{
				neighborRegionsSP[i] = buildCommand.m_neighbors[i].lock();
				neighborRegionsP[i] = neighborRegionsSP[i].get();
				if (neighborRegionsP[i] == nullptr)
				{
					anyNeighborNull = true;
					break;
				}
			}
			
			if (!anyNeighborNull)
			{
				BuildSync(*region, buildCommand.m_chunkY, neighborRegionsP, m_meshBuilder);
			}
		}
	}
}
