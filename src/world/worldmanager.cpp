#include "worldmanager.h"
#include "../rendering/regions/buildregionmesh.h"
#include "../rendering/regionrenderlist.h"
#include "../rendering/frustum.h"

#include <gsl/gsl_util>

namespace MCR
{
	WorldManager::WorldManager()
	    : m_generateThread(1)
	{
		SetRenderDistance(10);
	}
	
	void WorldManager::SaveAll(bool wait)
	{
		m_ioThread->BeginRegistering();
		
		for (int i = m_regionTableSize * m_regionTableSize - 1; i >= 0; i--)
		{
			const RegionEntry* entry = m_regions[0][i];
			if (entry != nullptr && entry->m_region.HasData())
			{
				m_ioThread->RegisterForSaving(entry->m_region);
			}
		}
		
		m_ioThread->EndRegistering();
		
		if (wait)
		{
			m_ioThread->WaitIdle();
		}
	}
	
	void WorldManager::SetRenderDistance(int renderDist)
	{
		m_hasUpdated = false;
		
		m_renderDistanceSq = renderDist * renderDist;
		m_loadDistance = renderDist + 1;
		
		m_regionTableSize = m_loadDistance * 2 + 1;
		const int numRegionEntries = m_regionTableSize * m_regionTableSize;
		
		//Allocates memory for the required number of regions.
		m_regionsAllocation = std::make_unique<RegionEntry[]>(numRegionEntries);
		
		//Fills the available regions list.
		m_availableRegions.resize(numRegionEntries);
		for (int i = 0; i < numRegionEntries; i++)
		{
			m_availableRegions[i] = &m_regionsAllocation[i];
		}
		
		//Initializes the region tables.
		for (int i = 0; i < 2; i++)
		{
			m_regions[i].resize(numRegionEntries);
			std::fill(MAKE_RANGE(m_regions[i]), nullptr);
		}
	}
	
	void WorldManager::Update(float dt, const class InputState& inputState)
	{
		if (m_world == nullptr)
			return;
		
		m_camera.Update(dt, inputState);
		
		const int64_t currentRegionX = std::floor(m_camera.GetPosition().x / Region::Size);
		const int64_t currentRegionZ = std::floor(m_camera.GetPosition().z / Region::Size);
		
		const int shiftX = gsl::narrow<int>(m_centerRegionX - currentRegionX);
		const int shiftZ = gsl::narrow<int>(m_centerRegionZ - currentRegionZ);
		
		const int64_t toGlobalX = currentRegionX - m_loadDistance;
		const int64_t toGlobalZ = currentRegionZ - m_loadDistance;
		
		constexpr bool enableIO = false;
		
		if (shiftX != 0 || shiftZ != 0 || !m_hasUpdated)
		{
			m_ioThread->BeginRegistering();
			m_generateThread.BeginRegistering();
			
			//Saves and frees regions that have been shifted off the region table
			//or moves the region to it's new location if hasn't been shifted off.
			if (m_hasUpdated)
			{
				for (int x = 0; x < m_regionTableSize; x++)
				{
					for (int z = 0; z < m_regionTableSize; z++)
					{
						int destIndex = GetRegionIndex(x + shiftX, z + shiftZ);
						
						RegionEntry* region = m_regions[0][GetRegionIndex(x, z)];
						if (region == nullptr)
							continue;
						
						if (destIndex == -1)
						{
							//This region has been pushed off the region table
							if (region->m_region.HasData() && enableIO)
							{
								m_ioThread->RegisterForSaving(std::move(region->m_region));
							}
							
							FreeRegionEntry(region);
						}
						else
						{
							m_regions[1][destIndex] = region;
						}
					}
				}
			}
			
			//Loads/generates the region if it was previouly out of bounds or if this is the first update
			//after changing world.
			for (int x = 0; x < m_regionTableSize; x++)
			{
				for (int z = 0; z < m_regionTableSize; z++)
				{
					int sourceIndex = GetRegionIndex(x, z);
					
					if (GetRegionIndex(x - shiftX, z - shiftZ) == -1 || !m_hasUpdated)
					{
						const RegionCoordinate coordinate = { x + toGlobalX, z + toGlobalZ };
						
						RegionEntry* region = AllocateRegionEntry();
						region->m_state = RegionStates::Loading;
						
						if (m_world->HasRegion(coordinate.x, coordinate.z) && enableIO)
						{
							m_ioThread->RegisterForLoading(coordinate);
						}
						else
						{
							m_generateThread.Register(coordinate);
						}
						
						m_regions[1][sourceIndex] = region;
					}
				}
			}
			
			m_ioThread->EndRegistering();
			m_generateThread.EndRegistering();
			
			m_regions[0].swap(m_regions[1]);
			m_centerRegionX = currentRegionX;
			m_centerRegionZ = currentRegionZ;
			
			m_hasUpdated = true;
		}
		
		//Processes loaded regions.
		auto ProcessNewRegion = [&] (Region& region)
		{
			const int regTableX = gsl::narrow<int>(region.GetX() - m_centerRegionX) + m_loadDistance;
			const int regTableZ = gsl::narrow<int>(region.GetZ() - m_centerRegionZ) + m_loadDistance;
			const int regTableIndex = GetRegionIndex(regTableX, regTableZ);
			
			RegionEntry* regionEntry = regTableIndex == -1 ? nullptr : m_regions[0][regTableIndex];
			
			if (regionEntry != nullptr)
			{
				regionEntry->m_state = RegionStates::LoadedNotBuilt;
				regionEntry->m_region = std::move(region);
			}
			else
			{
				Log("Loaded region rejected (", region.GetX(), ", ", region.GetZ(), ").");
			}
		};
		
		m_ioThread->IterateLoadedRegions(ProcessNewRegion);
		m_generateThread.IterateGeneratedRegions(ProcessNewRegion);
		
		const glm::vec2 regionNeighborDirs[] = 
		{
			/* NeighborPosX */  {  1, 0 },
			/* NeighborNegX */  { -1, 0 },
			/* NeighborPosZ */  { 0,  1 },
			/* NeighborNegZ */  { 0, -1 }
		};
		
		//Processes built regions
		m_regionBuildThread.IterateCompleted([&] (int64_t x, int64_t z, RegionDataBuffer& dataBuffer)
		{
			int localX = gsl::narrow<int>(x - m_centerRegionX) + m_loadDistance;
			int localZ = gsl::narrow<int>(z - m_centerRegionZ) + m_loadDistance;
			int regTableIndex = GetRegionIndex(localX, localZ);
			
			RegionEntry* regionEntry = regTableIndex == -1 ? nullptr : m_regions[0][regTableIndex];
			
			if (regionEntry != nullptr)
			{
				regionEntry->m_state = RegionStates::UpToDate;
				regionEntry->m_newMeshData.m_buffer = std::move(dataBuffer);
				regionEntry->m_mesh.SetData(std::move(regionEntry->m_newMeshData));
			}
			else
			{
				Log("Built region rejected (", x, ", ", z, ").");
			}
		});
		
		auto InitRegionBuildCommand = [] (RegionBuildThread::BuildCommand& buildCommand, RegionEntry& region)
		{
			buildCommand.m_region = &region.m_region;
			buildCommand.m_numIndicesOut = &region.m_newMeshData.m_numIndices;
			buildCommand.m_numVerticesOut = &region.m_newMeshData.m_numVertices;
			buildCommand.m_sliceOffsetsOut = region.m_newMeshData.m_sliceOffsets;
		};
		
		//Used to select a mesh to build synchronously this frame.
		struct
		{
			int x; //Local x coordinate
			int z; //Local z coordinate
			int distToCameraSq; //Distance to the current region squared.
		} regionToBuild;
		regionToBuild.distToCameraSq = std::numeric_limits<int>::max();
		
		//Manages the building of meshes.
		for (int x = 0; x < m_regionTableSize; x++)
		{
			for (int z = 0; z < m_regionTableSize; z++)
			{
				RegionEntry* region = m_regions[0][GetRegionIndex(x, z)];
				if (region == nullptr) //This probably never happens...
					continue;
				
				const int cameraDX = x - m_loadDistance;
				const int cameraDZ = z - m_loadDistance;
				const int distToCameraSq = cameraDX * cameraDX + cameraDZ * cameraDZ;
				const bool shouldHaveMesh = distToCameraSq < m_renderDistanceSq;
				
				if (shouldHaveMesh)
				{
					if (region->m_state == RegionStates::LoadedNotBuilt)
					{
						//This region is loaded and within range to have a mesh, but it doesn't have one
						//(this happens to regions that have just been loaded) so a build command should be submitted
						//to the build thread.
						
						RegionBuildThread::BuildCommand buildCommand;
						std::fill_n(buildCommand.m_neighbors, 4, nullptr);
						
						bool allNeighborsLoaded = true;
						for (int i = 0; i < 4; i++)
						{
							int neighborRegIndex = GetRegionIndex(x + regionNeighborDirs[i].x, z + regionNeighborDirs[i].y);
							if (neighborRegIndex == -1 || !m_regions[0][neighborRegIndex]->m_region.HasData())
							{
								allNeighborsLoaded = false;
								break;
							}
							else
							{
								buildCommand.m_neighbors[i] = &m_regions[0][neighborRegIndex]->m_region;
							}
						}
						
						if (allNeighborsLoaded)
						{
							InitRegionBuildCommand(buildCommand, *region);
							
							m_regionBuildThread.BuildASync(buildCommand);
							region->m_state = RegionStates::Building;
						}
					}
					else if (region->m_state == RegionStates::OutOfDate)
					{
						//This region is loaded and has a mesh, which is outdated. It is set as the region to build
						//synchronously this frame if it is closer to the camera than the currently selected region.
						if (distToCameraSq < regionToBuild.distToCameraSq)
						{
							regionToBuild.x = x;
							regionToBuild.z = z;
							regionToBuild.distToCameraSq = distToCameraSq;
						}
					}
				}
				else if (region->m_state == RegionStates::UpToDate || region->m_state == RegionStates::OutOfDate)
				{
					//This region has a mesh but is not within region to have one, so the mesh is discarded.
					region->m_mesh.Clear();
					region->m_state = RegionStates::LoadedNotBuilt;
				}
			}
		}
		
		//If any already built regions were out of date, builds the selected one and starts uploading it.
		if (regionToBuild.distToCameraSq != std::numeric_limits<int>::max())
		{
			RegionEntry* region = m_regions[0][GetRegionIndex(regionToBuild.x, regionToBuild.z)];
			region->m_state = RegionStates::Uploading;
			
			m_meshBuilder.Reset();
			
			RegionBuildThread::BuildCommand buildCommand;
			InitRegionBuildCommand(buildCommand, *region);
			
			for (int n = 0; n < 4; n++)
			{
				const int nx = regionToBuild.x + regionNeighborDirs[n].x;
				const int nz = regionToBuild.z + regionNeighborDirs[n].y;
				buildCommand.m_neighbors[n] = &m_regions[0][GetRegionIndex(nx, nz)]->m_region;
			}
			
			m_regionBuildThread.BuildSync(buildCommand, m_meshBuilder);
		}
	}
	
	void WorldManager::FillRenderListR(RegionRenderList& renderList, const Frustum& frustum,
	                                   int minX, int minZ, int spanX, int spanZ) const
	{
		if (spanX <= 0 || spanZ <= 0)
			return;
		
		const int64_t worldMinX = static_cast<int64_t>(minX - m_loadDistance) + m_centerRegionX;
		const int64_t worldMinZ = static_cast<int64_t>(minZ - m_loadDistance) + m_centerRegionZ;
		
		//The world space bounding box for the current span.
		AABoundingBox boundingBox(glm::vec3(worldMinX * Region::Size, 0, worldMinZ * Region::Size),
		                          glm::vec3((worldMinX + spanX) * Region::Size, Region::Height,
		                                    (worldMinZ + spanZ) * Region::Size));
		
		if (!frustum.Intersects(boundingBox))
			return;
		
		if (spanX == 1 && spanZ == 1)
		{
			//This span only consists of a single region, add it to the render list if it is in the correct state.
			
			RegionEntry* region = m_regions[0][GetRegionIndex(minX, minZ)];
			
			//Not sure if the null check is neccessary...
			if (region != nullptr && (region->m_state == RegionStates::UpToDate ||
			                          region->m_state == RegionStates::OutOfDate ||
			                          region->m_state == RegionStates::Uploading))
			{
				renderList.Add(region->m_mesh);
			}
		}
		else
		{
			//This span consists of multiple regions, subdivide the span further.
			
			//Sizes of the for 4 sub spans.
			const int subSpanX0 = std::max(spanX / 2, 1);
			const int subSpanZ0 = std::max(spanZ / 2, 1);
			const int subSpanX1 = spanX - subSpanX0;
			const int subSpanZ1 = spanZ - subSpanZ0;
			
			FillRenderListR(renderList, frustum, minX            , minZ,             subSpanX0, subSpanZ0);
			FillRenderListR(renderList, frustum, minX + subSpanX0, minZ,             subSpanX1, subSpanZ0);
			FillRenderListR(renderList, frustum, minX            , minZ + subSpanZ0, subSpanX0, subSpanZ1);
			FillRenderListR(renderList, frustum, minX + subSpanX0, minZ + subSpanZ0, subSpanX1, subSpanZ1);
		}
	}
	
	WorldManager::RegionEntry* WorldManager::AllocateRegionEntry()
	{
#ifndef MCR_DEBUG
		if (m_availableRegions.empty())
		{
			throw std::runtime_error("Out of region entries, this should not happen.");
		}
#endif
		
		RegionEntry* region = m_availableRegions.back();
		m_availableRegions.pop_back();
		return region;
	}
	
	void WorldManager::FreeRegionEntry(WorldManager::RegionEntry* entry)
	{
		*entry = RegionEntry();
		m_availableRegions.push_back(entry);
	}
	
	void WorldManager::SetWorld(std::unique_ptr<World> world)
	{
		m_world = std::move(world);
		
		if (m_world == nullptr)
		{
			m_ioThread = nullptr;
		}
		else
		{
			m_ioThread = std::make_unique<RegionIOThread>(*m_world);
		}
	}
}
