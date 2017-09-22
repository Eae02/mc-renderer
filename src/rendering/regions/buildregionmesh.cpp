#include "buildregionmesh.h"
#include "regionmesh.h"
#include "../../blocks/blocktype.h"
#include "../../blocks/sides.h"

namespace MCR
{
	void BuildRegionMesh(const RegionMeshBuildParams& params)
	{
#ifdef MCR_DEBUG
		if (params.m_slicesOut.size() != RegionMesh::NumSlices)
		{
			throw std::invalid_argument("Invalid number of slice offsets passed to BuildRegionMesh.");
		}
#endif
		
		for (int y = 0; y < Region::Height; y++)
		{
			if (y % Region::Size == 0)
			{
				params.m_slicesOut[y / Region::Size].m_indexOffset = params.m_meshBuilder->GetNumIndices();
			}
			
			for (int z = 0; z < Region::Size; z++)
			{
				const int64_t worldZ = z + params.m_region->GetZ() * Region::Size;
				
				for (int x = 0; x < Region::Size; x++)
				{
					const int64_t worldX = x + params.m_region->GetX() * Region::Size;
					
					Region::BlockEntry block = params.m_region->At(x, y, z);
					const BlockType& blockType = BlockType::GetByID(block.m_id);
					
					if (!blockType.IsInitialized())
						continue;
					
					if (const ICustomMeshProvider* meshProvider = blockType.GetCustomMeshProvider())
					{
						meshProvider->BuildBlockMesh(*params.m_meshBuilder, worldX, y, worldZ, block.m_data);
						continue;
					}
					
					const glm::vec3 blockWorldCenter(worldX + 0.5f, y + 0.5f, worldZ + 0.5f);
					
					for (int s = 0; s < 6; s++)
					{
						glm::ivec3 frontBlockPos = glm::ivec3(x, y, z) + BlockNormals[s];
						
						if (frontBlockPos.y < 0)
							continue;
						
						const Region* frontRegion = params.m_region;
						
						//Modulates the front block position along the X-axis.
						if (frontBlockPos.x < 0)
						{
							frontRegion = params.m_neighbors[NeighborNegX];
							frontBlockPos.x += Region::Size;
						}
						else if (frontBlockPos.x >= Region::Size)
						{
							frontRegion = params.m_neighbors[NeighborPosX];
							frontBlockPos.x -= Region::Size;
						}
						
						//Modulates the front block position along the Z-axis.
						if (frontBlockPos.z < 0)
						{
							frontRegion = params.m_neighbors[NeighborNegZ];
							frontBlockPos.z += Region::Size;
						}
						else if (frontBlockPos.z >= Region::Size)
						{
							frontRegion = params.m_neighbors[NeighborPosZ];
							frontBlockPos.z -= Region::Size;
						}
						
						//Checks if the front block is opaque.
						if (frontBlockPos.y < Region::Height)
						{
							uint8_t frontBlockID = frontRegion->At(frontBlockPos.x, frontBlockPos.y, frontBlockPos.z).m_id;
							const BlockType& frontBlockType = BlockType::GetByID(frontBlockID);
							
							if (frontBlockType.IsOpaque())
								continue;
						}
						
						const glm::vec3 faceCenter = blockWorldCenter + glm::vec3(BlockNormals[s]) * 0.5f;
						
						const int layer = blockType.GetTextureLayer(s);
						
						const uint32_t baseIndex = params.m_meshBuilder->GetNextVertexIndex();
						params.m_meshBuilder->AddTriangle(baseIndex + 0, baseIndex + 1, baseIndex + 2);
						params.m_meshBuilder->AddTriangle(baseIndex + 2, baseIndex + 1, baseIndex + 3);
						
						const glm::vec3 up = BlockTangents[s];
						const glm::vec3 left = BlockBiTangents[s];
						
						for (int vx = 0; vx < 2; vx++)
						{
							for (int vy = 0; vy < 2; vy++)
							{
								const glm::vec3 pos = faceCenter + up * (vy - 0.5f) + left * (vx - 0.5f);
								params.m_meshBuilder->AddVertex(pos, { vx, 1 - vy }, layer);
							}
						}
					}
				}
			}
		}
	}
}
