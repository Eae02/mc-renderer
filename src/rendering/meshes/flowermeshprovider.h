#pragma once

#include "icustommeshprovider.h"

#include <string_view>

namespace MCR
{
	class FlowerMeshProvider : public ICustomMeshProvider
	{
	public:
		FlowerMeshProvider(std::string_view textureName, float size);
		
		void BuildBlockMesh(class MeshBuilder& meshBuilder, int64_t x, int64_t y, int64_t z, uint8_t blockData) const;
		
	private:
		float m_size;
		int m_texLayer;
	};
}
