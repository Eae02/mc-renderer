#pragma once

#include "meshbuilder.h"
#include "watermeshbuilder.h"
#include "chunkmesh.h"
#include "../../world/region.h"

namespace MCR
{
	enum RegionNeighbors
	{
		NeighborPosX,
		NeighborNegX,
		NeighborPosZ,
		NeighborNegZ,
	};
	
	struct ChunkMeshBuildParams
	{
		const Region* m_region;
		const Region* m_neighbors[4]; //Indexed using RegionNeighbors
		uint32_t m_chunkY;
		MeshBuilder* m_meshBuilder;
	};
	
	void BuildChunkMesh(const ChunkMeshBuildParams& params);
	
	void BuildWaterMesh(const Region& m_region, uint32_t chunkY, WaterMeshBuilder& waterMeshBuilder);
}
