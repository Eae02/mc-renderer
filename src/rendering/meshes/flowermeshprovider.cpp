#include "flowermeshprovider.h"
#include "../regions/meshbuilder.h"
#include "../../blocks/blockstexturemanager.h"

namespace MCR
{
	FlowerMeshProvider::FlowerMeshProvider(std::string_view textureName)
	    : m_texLayer(BlocksTextureManager::GetInstance().GetAlbedoTextureIndex(textureName)) { }
	
	void FlowerMeshProvider::BuildBlockMesh(MeshBuilder& meshBuilder, int64_t x, int64_t y, int64_t z,
	                                        uint8_t blockData) const
	{
		const float distFromCenter = 0.4f;
		const float height = distFromCenter * 2;
		
		const glm::vec3 centerBtm(x + 0.5f, y, z + 0.5f);
		
		uint32_t baseIndex = meshBuilder.GetNextVertexIndex();
		
		const glm::vec2 sideDirections[] = 
		{
			glm::vec2(1, 1),
			glm::vec2(-1, 1)
		};
		
		for (const glm::vec2& sideDir : sideDirections)
		{
			for (int a = 0; a < 2; a++)
			{
				const float sideOffset = distFromCenter * (a * 2 - 1);
				const glm::vec3 offset(sideDir.x * sideOffset, 0, sideDir.y * sideOffset);
				
				for (int b = 0; b < 2; b++)
				{
					meshBuilder.AddVertex(centerBtm + offset + glm::vec3(0, b * height, 0), glm::vec3(0, 1, 0),
					                      glm::vec3(1, 0, 0), glm::vec2(a, 1 - b), m_texLayer, -1, 1.0f);
				}
			}
		}
		
		const uint32_t indices[] = 
		{
			0, 1, 2,
			1, 3, 2,
			2, 1, 0,
			2, 3, 1
		};
		
		for (uint32_t s = 0; s < 2; s++)
		{
			for (size_t i = 0; i < ArrayLength(indices); i += 3)
			{
				meshBuilder.AddTriangle(baseIndex + indices[i], baseIndex + indices[i + 1], baseIndex + indices[i + 2]);
			}
			baseIndex += 4;
		}
	}
}
