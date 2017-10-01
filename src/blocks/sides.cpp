#include "sides.h"

namespace MCR
{
	const uint8_t OpposingSides[6] = 
	{
		/* BLOCK_SIDE_POSX -> */ BLOCK_SIDE_NEGX,
		/* BLOCK_SIDE_NEGX -> */ BLOCK_SIDE_POSX,
		/* BLOCK_SIDE_POSY -> */ BLOCK_SIDE_NEGY,
		/* BLOCK_SIDE_NEGY -> */ BLOCK_SIDE_POSY,
		/* BLOCK_SIDE_POSZ -> */ BLOCK_SIDE_NEGZ,
		/* BLOCK_SIDE_NEGZ -> */ BLOCK_SIDE_POSZ
	};
	
	const glm::ivec3 BlockNormals[6] =
	{
		{  1, 0, 0 },
		{ -1, 0, 0 },
		{ 0,  1, 0 },
		{ 0, -1, 0 },
		{ 0, 0,  1 },
		{ 0, 0, -1 }
	};
	
	const glm::ivec3 BlockTangents[6] =
	{
		{ 0, 1, 0 },
		{ 0, 1, 0 },
		{ 1, 0, 0 },
		{ 1, 0, 0 },
		{ 0, 1, 0 },
		{ 0, 1, 0 }
	};
	
	const glm::ivec3 BlockBiTangents[6] =
	{
		{ 0, 0, -1 },
		{ 0, 0, 1 },
		{ 0, 0, 1 },
		{ 0, 0, -1 },
		{ 1, 0, 0 },
		{ -1, 0, 0 }
	};
}
