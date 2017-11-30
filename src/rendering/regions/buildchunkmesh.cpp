#include "buildchunkmesh.h"
#include "../../blocks/blocktype.h"
#include "../../blocks/ids.h"
#include "watermesh.h"

namespace MCR
{
	void BuildChunkMesh(const ChunkMeshBuildParams& params)
	{
		const int64_t baseWorldY = params.m_chunkY * Region::Size;
		
		for (int yo = 0; yo < Region::Size; yo++)
		{
			const int64_t y = yo + baseWorldY;
			
			for (int z = 0; z < Region::Size; z++)
			{
				const int64_t worldZ = z + params.m_region->GetZ() * Region::Size;
				
				for (int x = 0; x < Region::Size; x++)
				{
					const int64_t worldX = x + params.m_region->GetX() * Region::Size;
					
					Region::BlockEntry block = params.m_region->Get(x, y, z);
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
							uint8_t frontBlockID = frontRegion->Get(frontBlockPos.x, frontBlockPos.y, frontBlockPos.z).m_id;
							const BlockType& frontBlockType = BlockType::GetByID(frontBlockID);
							
							if (frontBlockType.IsOpaque())
								continue;
						}
						
						const glm::vec3 faceCenter = blockWorldCenter + glm::vec3(BlockNormals[s]) * 0.5f;
						
						const int albedoLayer = blockType.GetAlbedoTextureLayer(s);
						const int normalLayer = blockType.GetNormalTextureLayer(s);
						
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
								params.m_meshBuilder->AddVertex(pos, BlockNormals[s], left, { vx, 1 - vy },
								                                albedoLayer, normalLayer, blockType.GetRoughness(),
								                                blockType.GetBendiness());
							}
						}
					}
				}
			}
		}
	}
	
	void BuildWaterMesh(const Region& region, uint32_t chunkY, WaterMeshBuilder& waterMeshBuilder)
	{
		const int baseWorldY = chunkY * Region::Size;
		
		std::array<int, Region::Size * Region::Size> lastOpaqueBlockYTable;
		std::fill(MAKE_RANGE(lastOpaqueBlockYTable), -1);
		
		std::vector<glm::vec2> polygonVertices;
		
		for (int yo = 0; yo < Region::Size; yo++)
		{
			std::bitset<Region::Size * Region::Size> hasWater;
			
			//Detects water
			const int y = yo + baseWorldY;
			for (int z = 0; z < Region::Size; z++)
			{
				const int64_t worldZ = z + region.GetZ() * Region::Size;
				
				for (int x = 0; x < Region::Size; x++)
				{
					const int64_t worldX = x + region.GetX() * Region::Size;
					
					Region::BlockEntry block = region.Get(x, y, z);
					
					if (BlockType::GetByID(block.m_id).IsOpaque())
					{
						lastOpaqueBlockYTable[z * Region::Size + x] = y;
					}
					//If this block is water and the block above is air, insert a water quad.
					else if (block.m_id == BlockIDs::Water &&
					         (y == Region::Height - 1 || region.Get(x, y + 1, z).m_id == BlockIDs::Air))
					{
						hasWater.set(static_cast<size_t>(z * Region::Size + x));
					}
				}
			}
			
			if (!hasWater.any())
				continue;
			
			const float waterY = y + WaterMesh::WaterHeight;
			
			auto GetLastOpaqueBlockY = [&] (int64_t x, int64_t z)
			{
				int& lastOpaqueBlockY = lastOpaqueBlockYTable[z * Region::Size + x];
				
				if (lastOpaqueBlockY == -1)
				{
					lastOpaqueBlockY = baseWorldY - 1;
					while (lastOpaqueBlockY > 0 &&
					       !BlockType::GetByID(region.Get(x, lastOpaqueBlockY, z).m_id).IsOpaque())
					{
						lastOpaqueBlockY--;
					}
				}
				
				return lastOpaqueBlockY;
			};
			
			auto GetWaterDepth = [&] (int64_t x, int64_t z)
			{
				float depth = 0;
				for (int64_t ox = -1; ox <= 0; ox++)
				{
					int64_t px = x + ox;
					if (px < 0 || px >= Region::Size)
						continue;
					
					for (int64_t oz = -1; oz <= 0; oz++)
					{
						int64_t pz = z + oz;
						if (pz < 0 || pz >= Region::Size)
							continue;
						depth = std::max(depth, waterY - GetLastOpaqueBlockY(px, pz));
					}
				}
				return depth;
			};
			
			int64_t minWorldX = region.GetX() * Region::Size;
			int64_t minWorldZ = region.GetZ() * Region::Size;
			
			const int waterQuadSize = 16;
			const int numQuads = Region::Size / waterQuadSize;
			
			int quadVertexIndices[numQuads + 1][numQuads + 1];
			for (int x = 0; x <= numQuads; x++)
			{
				for (int z = 0; z <= numQuads; z++)
				{
					quadVertexIndices[x][z] = -1;
				}
			}
			
			auto QuadHasWater = [&] (int qx, int qz)
			{
				int mx = qx * waterQuadSize;
				int mz = qz * waterQuadSize;
				
				for (int lz = 0; lz < waterQuadSize; lz++)
				{
					for (int lx = 0; lx < waterQuadSize; lx++)
					{
						if (hasWater[(lz + mz) * Region::Size + lx + mx])
							return true;
					}
				}
				
				return false;
			};
			
			for (int qx = 0; qx < numQuads; qx++)
			{
				for (int qz = 0; qz < numQuads; qz++)
				{
					if (!QuadHasWater(qx, qz))
						continue;
					
					uint16_t indices[2][2];
					for (int ox = 0; ox < 2; ox++)
					{
						int vx = (qx + ox) * waterQuadSize;
						
						for (int oz = 0; oz < 2; oz++)
						{
							int vz = (qz + oz) * waterQuadSize;
							
							int& index = quadVertexIndices[qx + ox][qz + oz];
							
							if (index == -1)
							{
								index = waterMeshBuilder.GetNextIndex();
								waterMeshBuilder.AddVertex(
									{
										glm::vec3(minWorldX + vx, waterY, minWorldZ + vz), GetWaterDepth(vx, vz)
									}
								);
							}
							
							indices[ox][oz] = static_cast<uint16_t>(index);
						}
					}
					
					waterMeshBuilder.AddTriangle(indices[0][0], indices[1][0], indices[0][1]);
					waterMeshBuilder.AddTriangle(indices[1][0], indices[1][1], indices[0][1]);
				}
			}
		}
	}
}
