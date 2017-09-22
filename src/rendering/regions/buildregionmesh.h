#pragma once

#include "meshbuilder.h"
#include "regionmesh.h"
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
	
	struct RegionMeshBuildParams
	{
		const Region* m_region;
		const Region* m_neighbors[4]; //Indexed using RegionNeighbors
		MeshBuilder* m_meshBuilder;
		gsl::span<RegionMesh::SliceData> m_slicesOut;
	};
	
	void BuildRegionMesh(const RegionMeshBuildParams& buildParams);
}
