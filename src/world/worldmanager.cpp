#include "worldmanager.h"
#include "../rendering/chunkrenderlist.h"
#include "../rendering/frustum.h"
#include "../rendering/regions/buildchunkmesh.h"
#include "../blocks/sides.h"

#include <gsl/gsl_util>

namespace MCR
{
	WorldManager::WorldManager()
	    : m_generateThread(4)
	{
		SetRenderDistance(8);
	}
	
	constexpr bool enableIO = false;
	
	void WorldManager::SaveAll()
	{
		if (!enableIO)
			return;
		
		m_ioThread->BeginRegistering();
		
		for (int i = m_regionTableSize * m_regionTableSize - 1; i >= 0; i--)
		{
			const RegionEntry* entry = m_regions[0][i];
			if (entry != nullptr && entry->m_region)
			{
				m_ioThread->RegisterForSaving(entry->m_region);
			}
		}
		
		m_ioThread->EndRegistering();
	}
	
	void WorldManager::WaitIdle()
	{
		if (enableIO)
		{
			m_ioThread->WaitIdle();
		}
	}
	
	void WorldManager::SetRenderDistance(int renderDist)
	{
		m_hasUpdated = false;
		
		m_renderDistanceSq = renderDist * renderDist;
		m_loadDistance = renderDist + 3;
		
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
		
		const int64_t currentChunkY = std::floor(m_camera.GetPosition().y / Region::Size);
		
		const int shiftX = gsl::narrow<int>(m_centerRegionX - currentRegionX);
		const int shiftZ = gsl::narrow<int>(m_centerRegionZ - currentRegionZ);
		
		const int64_t toGlobalX = currentRegionX - m_loadDistance;
		const int64_t toGlobalZ = currentRegionZ - m_loadDistance;
		
		const bool shifted = shiftX != 0 || shiftZ != 0 || !m_hasUpdated;
		
		if (shifted)
		{
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
							if (region->m_region && enableIO)
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
			
			m_ioThread->BeginRegistering();
			m_generateThread.BeginRegistering();
			
			m_generateThread.SetCameraRegion({ currentRegionX, currentRegionZ });
			
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
		auto ProcessNewRegion = [&] (NewRegion& newRegion)
		{
			const int regTableX = gsl::narrow<int>(newRegion.m_region->GetX() - m_centerRegionX) + m_loadDistance;
			const int regTableZ = gsl::narrow<int>(newRegion.m_region->GetZ() - m_centerRegionZ) + m_loadDistance;
			const int regTableIndex = GetRegionIndex(regTableX, regTableZ);
			
			RegionEntry* regionEntry = regTableIndex == -1 ? nullptr : m_regions[0][regTableIndex];
			
			if (regionEntry != nullptr)
			{
				for (uint32_t i = 0; i < Region::ChunkCount; i++)
				{
					if (newRegion.m_region->ChunkHasWater(i))
					{
						m_outOfDateWaterList.push_back({{newRegion.m_region->GetX(), newRegion.m_region->GetZ()}, i});
					}
				}
				
				regionEntry->m_state = RegionStates::LoadedNotBuilt;
				regionEntry->m_region = std::move(newRegion.m_region);
			}
		};
		
		if (enableIO)
		{
			m_ioThread->IterateLoadedRegions(ProcessNewRegion);
		}
		
		if (m_generateThread.IterateGeneratedRegions(ProcessNewRegion))
		{
			m_generateThread.ProcessFutureRegions(*this);
		}
		
		const glm::ivec2 regionNeighborDirs[] = 
		{
			/* NeighborPosX */  {  1, 0 },
			/* NeighborNegX */  { -1, 0 },
			/* NeighborPosZ */  { 0,  1 },
			/* NeighborNegZ */  { 0, -1 }
		};
		
		//Processes built regions
		m_chunkBuildThread.IterateCompleted([&] (int64_t x, int64_t y, int64_t z, ChunkMesh& mesh)
		{
			int localX = gsl::narrow<int>(x - m_centerRegionX) + m_loadDistance;
			int localZ = gsl::narrow<int>(z - m_centerRegionZ) + m_loadDistance;
			int regTableIndex = GetRegionIndex(localX, localZ);
			
			RegionEntry* regionEntry = regTableIndex == -1 ? nullptr : m_regions[0][regTableIndex];
			
			if (regionEntry != nullptr)
			{
				regionEntry->m_state = RegionStates::Built;
				regionEntry->m_meshes[y] = std::move(mesh);
			}
		});
		
		//Used to select a chunk mesh to build synchronously this frame.
		struct
		{
			int x; //Local x coordinate
			uint32_t y; //Chunk y coordinate
			int z; //Local z coordinate
			int distToCameraSq; //Distance to the current region squared.
		} chunkToBuild;
		chunkToBuild.distToCameraSq = std::numeric_limits<int>::max();
		
		bool buildThreadUpdating = false;
		
		//Updates the build thread's camera region
		if (shifted)
		{
			m_chunkBuildThread.BeginUpdating();
			m_chunkBuildThread.SetCameraPosition(currentRegionX, currentChunkY, currentRegionZ);
			buildThreadUpdating = true;
		}
		
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
						
						ChunkBuildThread::BuildCommand buildCommand;
						
						bool allNeighborsLoaded = true;
						for (int i = 0; i < 4; i++)
						{
							int neighborRegIndex = GetRegionIndex(x + regionNeighborDirs[i].x,
							                                      z + regionNeighborDirs[i].y);
							if (neighborRegIndex == -1 || !m_regions[0][neighborRegIndex]->m_region)
							{
								allNeighborsLoaded = false;
								break;
							}
							else
							{
								buildCommand.m_neighbors[i] = m_regions[0][neighborRegIndex]->m_region;
							}
						}
						
						if (allNeighborsLoaded)
						{
							buildCommand.m_coordinate = { region->m_region->GetX(), region->m_region->GetZ() };
							buildCommand.m_region = region->m_region;
							
							if (!buildThreadUpdating)
							{
								m_chunkBuildThread.BeginUpdating();
								buildThreadUpdating = true;
							}
							
							for (uint32_t y = 0; y < Region::ChunkCount; y++)
							{
								buildCommand.m_chunkY = y;
								m_chunkBuildThread.BuildASync(buildCommand);
							}
							
							region->m_state = RegionStates::Building;
						}
					}
					else if (region->m_state == RegionStates::Built && region->m_meshesOutOfDate.any())
					{
						for (uint32_t y = 0; y < Region::ChunkCount; y++)
						{
							if (region->m_meshesOutOfDate[y])
							{
								const int cameraDY = static_cast<int>(y) - currentChunkY;
								const int distToChunkSq = distToCameraSq + cameraDY * cameraDY;
								
								//This chunk is loaded and has a mesh, which is outdated. It is set as the chunk to
								//build synchronously this frame if it is closer to the camera than the currently
								//selected chunk.
								if (distToChunkSq < chunkToBuild.distToCameraSq)
								{
									chunkToBuild.x = x;
									chunkToBuild.y = y;
									chunkToBuild.z = z;
									chunkToBuild.distToCameraSq = distToCameraSq;
								}
							}
						}
					}
				}
				else if (region->m_state == RegionStates::Built)
				{
					//This region's chunks have meshes but it not within the required region, so the meshes are discarded.
					for (ChunkMesh& mesh : region->m_meshes)
					{
						mesh.Reset();
					}
					
					region->m_meshesOutOfDate = { };
					region->m_state = RegionStates::LoadedNotBuilt;
				}
			}
		}
		
		if (buildThreadUpdating)
		{
			m_chunkBuildThread.EndUpdating();
		}
		
		//If any already built chunks were out of date, builds the selected one and starts uploading it.
		if (chunkToBuild.distToCameraSq != std::numeric_limits<int>::max())
		{
			RegionEntry* region = m_regions[0][GetRegionIndex(chunkToBuild.x, chunkToBuild.z)];
			
			region->m_state = RegionStates::Uploading;
			region->m_meshesOutOfDate.reset(chunkToBuild.y);
			
			std::array<const Region*, 4> neighbors;
			for (int n = 0; n < 4; n++)
			{
				const int nx = chunkToBuild.x + regionNeighborDirs[n].x;
				const int nz = chunkToBuild.z + regionNeighborDirs[n].y;
				neighbors[n] = m_regions[0][GetRegionIndex(nx, nz)]->m_region.get();
			}
			
			m_chunkBuildThread.BuildSync(*region->m_region, chunkToBuild.y, neighbors, m_meshBuilder);
		}
	}
	
	WorldManager::MeshRenderInfo WorldManager::GetChunkMeshRenderInfo(int64_t x, int y, int64_t z) const
	{
		WorldManager::MeshRenderInfo info = { nullptr, nullptr };
		
		int regionIndex = GetRegionIndex(static_cast<int>(x - m_centerRegionX + m_loadDistance),
		                                 static_cast<int>(z - m_centerRegionZ + m_loadDistance));
		if (regionIndex == -1)
			return info;
		
		RegionEntry* region = m_regions[0][regionIndex];
		if (region == nullptr || region->m_region == nullptr)
			return info;
		
		if (region->m_waterMeshes[y].HasData())
		{
			info.m_waterMesh = &region->m_waterMeshes[y];
		}
		
		if (region->m_state == RegionStates::Built || region->m_state == RegionStates::Uploading)
		{
			info.m_chunkMesh = &region->m_meshes[y];
		}
		
		return info;
	}
	
	void WorldManager::FillRenderListR(ChunkRenderList& renderList, const Frustum& frustum,
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
			
			//Not sure if the null check is necessary...
			if (region != nullptr && (region->m_state == RegionStates::Built ||
			                          region->m_state == RegionStates::Uploading))
			{
				for (ChunkMesh& mesh : region->m_meshes)
				{
					if (mesh.HasData())
					{
						renderList.Add(mesh);
					}
				}
				
				for (WaterMesh& waterMesh : region->m_waterMeshes)
				{
					if (!waterMesh.Empty())
					{
						renderList.Add(waterMesh);
					}
				}
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
	
	WorldManager::RegionEntry* WorldManager::RegionEntryFromGlobalCoordinate(RegionCoordinate coordinate)
	{
		const int index = GetRegionIndex(static_cast<int>(coordinate.x - m_centerRegionX + m_loadDistance),
		                                 static_cast<int>(coordinate.z - m_centerRegionZ + m_loadDistance));
		
		return index == -1 ? nullptr : m_regions[0][index];
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
	
	Region* WorldManager::GetRegion(RegionCoordinate coordinate)
	{
		RegionEntry* entry = RegionEntryFromGlobalCoordinate(coordinate);
		
		if (entry == nullptr || entry->m_state == RegionStates::Loading)
			return nullptr;
		
		return entry->m_region.get();
	}
	
	void WorldManager::MarkOutOfDate(RegionCoordinate coordinate, uint32_t chunkY)
	{
		RegionEntry* entry = RegionEntryFromGlobalCoordinate(coordinate);
		
		if (entry != nullptr && entry->m_state == RegionStates::Built)
		{
			entry->m_meshesOutOfDate.set(chunkY);
		}
	}
	
	void WorldManager::BuildWater(CommandBuffer& commandBuffer)
	{
		for (const OutOfDateWaterMesh& waterMesh : m_outOfDateWaterList)
		{
			RegionEntry* regionEntry = RegionEntryFromGlobalCoordinate(waterMesh.m_coordinate);
			if (regionEntry == nullptr)
				continue;
			
			m_waterMeshBuilder.Reset();
			BuildWaterMesh(*regionEntry->m_region, waterMesh.m_chunkY, m_waterMeshBuilder);
			
			if (m_waterMeshBuilder.Empty())
				continue;
			
			regionEntry->m_waterMeshes[waterMesh.m_chunkY].Upload(commandBuffer, m_waterMeshBuilder.GetVertices(),
			                                                      m_waterMeshBuilder.GetIndices());
		}
		
		m_outOfDateWaterList.clear();
	}
}
