#pragma once

#include <libnoise/module/perlin.h>

#include "region.h"

namespace MCR
{
	class WorldGenerator
	{
	public:
		WorldGenerator();
		
		void SetSeed(int seed);
		
		Region Generate(int64_t rx, int64_t rz) const;
		
	private:
		noise::module::Perlin m_heightPerlin;
		noise::module::Perlin m_flowerPerlin;
	};
}
