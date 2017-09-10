#include "worldgenerator.h"
#include "../blocks/ids.h"

#include <random>

namespace MCR
{
	WorldGenerator::WorldGenerator()
	{
		m_heightPerlin.SetLacunarity(2.0);
		m_heightPerlin.SetPersistence(0.5);
		m_heightPerlin.SetOctaveCount(2);
		m_heightPerlin.SetNoiseQuality(noise::QUALITY_STD);
	}
	
	void WorldGenerator::SetSeed(int seed)
	{
		m_heightPerlin.SetSeed(seed);
	}
	
	Region WorldGenerator::Generate(int64_t rx, int64_t rz) const
	{
		Region region(rx, rz);
		region.SetHasData(true);
		
		double PerlinDiv = 25;
		
		const int averageSurfaceLevel = 64;
		const double surfaceLevelRange = 10;
		
		for (int lz = 0; lz < Region::Size; lz++)
		{
			int64_t z = lz + rz * Region::Size;
			
			for (int lx = 0; lx < Region::Size; lx++)
			{
				int64_t x = lx + rx * Region::Size;
				
				int blocksSinceAir = 0;
				
				for (int y = Region::Height - 1; y >= 0; y--)
				{
					Region::BlockEntry& block = region.At(lx, y, lz);
					
					block.m_data = 0;
					
					double val = m_heightPerlin.GetValue(x / PerlinDiv, y / PerlinDiv, z / PerlinDiv);
					val += (y - averageSurfaceLevel) / surfaceLevelRange;
					
					if (val < 0)
					{
						if (blocksSinceAir == 0)
							block.m_id = BlockIDs::Grass;
						else if (blocksSinceAir > 3)
							block.m_id = BlockIDs::Stone;
						else
							block.m_id = BlockIDs::Dirt;
					}
					else
					{
						block.m_id = BlockIDs::Air;
					}
					
					if (block.m_id == BlockIDs::Air)
						blocksSinceAir = 0;
					else
						blocksSinceAir++;
				}
			}
		}
		
		return region;
	}
}
