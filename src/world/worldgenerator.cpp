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
		
		m_flowerPerlin.SetFrequency(16);
		m_flowerPerlin.SetLacunarity(2.0);
		m_flowerPerlin.SetPersistence(0.5);
		m_flowerPerlin.SetOctaveCount(2);
		m_flowerPerlin.SetNoiseQuality(noise::QUALITY_FAST);
		
		SetSeed(0);
	}
	
	void WorldGenerator::SetSeed(int seed)
	{
		std::mt19937 rand(seed);
		
		m_heightPerlin.SetSeed(rand());
		m_flowerPerlin.SetSeed(rand());
	}
	
	Region WorldGenerator::Generate(int64_t rx, int64_t rz) const
	{
		Region region(rx, rz);
		region.SetHasData(true);
		
		double PerlinDiv = 25;
		
		const int averageSurfaceLevel = 64;
		const double surfaceLevelRange = 10;
		
		//Average number of surface blocks with flowers as a percentage.
		const double flowerFrequency = 0.4;
		
		const uint8_t flowerIDs[] = { BlockIDs::BlueFlower, BlockIDs::RedFlower, BlockIDs::YellowFlower, BlockIDs::OrangeFlower, BlockIDs::WhiteFlower };
		
		for (int lz = 0; lz < Region::Size; lz++)
		{
			int64_t z = lz + rz * Region::Size;
			double pz = z / PerlinDiv;
			
			for (int lx = 0; lx < Region::Size; lx++)
			{
				int64_t x = lx + rx * Region::Size;
				double px = x / PerlinDiv;
				
				int blocksSinceAir = 0;
				
				for (int y = Region::Height - 1; y >= 0; y--)
				{
					Region::BlockEntry& block = region.At(lx, y, lz);
					
					block.m_data = 0;
					
					double py = y / PerlinDiv;
					
					double val = m_heightPerlin.GetValue(px, py, pz);
					val += (y - averageSurfaceLevel) / surfaceLevelRange;
					
					if (val < 0)
					{
						if (blocksSinceAir == 0)
						{
							block.m_id = BlockIDs::Grass;
							
							if (y < Region::Height - 1)
							{
								double flowerVal = m_flowerPerlin.GetValue(px, py, pz) * 0.5 + 0.5;
								
								if (flowerVal < flowerFrequency)
								{
									double flowerIndexD = std::floor(flowerVal / flowerFrequency * ArrayLength(flowerIDs));
									
									region.At(lx, y + 1, lz).m_id = flowerIDs[static_cast<int>(flowerIndexD)];
								}
							}
						}
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
