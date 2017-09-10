#pragma once

#include <glm/glm.hpp>

namespace MCR
{
	enum
	{
		BLOCK_SIDE_POSX,
		BLOCK_SIDE_NEGX,
		BLOCK_SIDE_POSY,
		BLOCK_SIDE_NEGY,
		BLOCK_SIDE_POSZ,
		BLOCK_SIDE_NEGZ
	};
	
	extern const glm::ivec3 BlockNormals[6];
	
	extern const glm::ivec3 BlockTangents[6];
	
	extern const glm::ivec3 BlockBiTangents[6];
}
