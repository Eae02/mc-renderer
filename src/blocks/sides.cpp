#include "sides.h"

namespace MCR
{
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
