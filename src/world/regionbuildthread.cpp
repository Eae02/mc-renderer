#include "regionbuildthread.h"
#include "region.h"
#include "../rendering/regions/buildregionmesh.h"

namespace MCR
{
	RegionBuildThread::RegionBuildThread()
	    : m_thread(&RegionBuildThread::ThreadTarget, this)
	{
		
	}
	
	RegionBuildThread::~RegionBuildThread()
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_exit = true;
			m_signal.notify_one();
		}
		
		m_thread.join();
	}
	
	void RegionBuildThread::BuildSync(const Region& region, gsl::span<const Region*> neighbors,
	                                  MeshBuilder& meshBuilder)
	{
		std::array<uint32_t, Region::SliceCount> sliceOffsets;
		
		RegionMeshBuildParams buildParams;
		buildParams.m_meshBuilder = &meshBuilder;
		buildParams.m_region = &region;
		buildParams.m_sliceOffsets = sliceOffsets;
		std::copy_n(neighbors.begin(), 4, buildParams.m_neighbors);
		
		meshBuilder.Reset();
		
		BuildRegionMesh(buildParams);
		
		m_uploader.BeginUploading(region.GetX(), region.GetZ(), sliceOffsets, meshBuilder);
	}
	
	void RegionBuildThread::ThreadTarget()
	{
		while (true)
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			
			m_signal.wait(lock, [&] { return !m_buildCommands.empty() || m_exit; });
			
			if (m_exit)
				break;
			
			//Selects the closest region to the camera for building.
			ssize_t selectedIndex = -1;
			uint64_t selectedRegionDistFromCamera = 0;
			for (size_t i = 0; i < m_buildCommands.size(); i++)
			{
				uint64_t distFromCamera = RegionCoordinate::DistanceSq(m_buildCommands[i].m_coordinate, m_cameraRegion);
				if (selectedRegionDistFromCamera > distFromCamera || selectedIndex == -1)
				{
					selectedIndex = i;
					selectedRegionDistFromCamera = distFromCamera;
				}
			}
			
			BuildCommand buildCommand = std::move(m_buildCommands[selectedIndex]);
			
			if (selectedIndex != static_cast<ssize_t>(m_buildCommands.size()) - 1)
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
				BuildSync(*region, neighborRegionsP, m_meshBuilder);
			}
		}
	}
}
