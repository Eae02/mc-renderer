#pragma once

#include <cstdint>

#include "../../abstract.h"

namespace MCR
{
	class ICustomMeshProvider : public Abstract
	{
	public:
		virtual void BuildBlockMesh(class MeshBuilder& meshBuilder, int64_t x, int64_t y, int64_t z,
		                            uint8_t blockData) const = 0;
	};
}
