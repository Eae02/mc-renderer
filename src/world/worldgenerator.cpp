#include "worldgenerator.h"
#include "worldmanager.h"
#include "../blocks/ids.h"

#include <random>
#include <glm/gtc/constants.hpp>

#include <byteswap.h>

namespace MCR
{
	WorldGenerator::WorldGenerator()
	{
		m_roughnessPerlin.SetFrequency(1.3);
		m_roughnessPerlin.SetLacunarity(2.0);
		m_roughnessPerlin.SetPersistence(0.5);
		m_roughnessPerlin.SetOctaveCount(2);
		m_roughnessPerlin.SetNoiseQuality(noise::QUALITY_FAST);
		
		m_heightPerlin.SetFrequency(1.0 / 4.0);
		m_heightPerlin.SetLacunarity(2.0);
		m_heightPerlin.SetPersistence(0.5);
		m_heightPerlin.SetOctaveCount(4);
		m_heightPerlin.SetNoiseQuality(noise::QUALITY_STD);
		
		m_terracePerlin.SetFrequency(1.0 / 4.0);
		m_terracePerlin.SetLacunarity(2.0);
		m_terracePerlin.SetPersistence(0.5);
		m_terracePerlin.SetOctaveCount(2);
		m_terracePerlin.SetNoiseQuality(noise::QUALITY_STD);
		
		m_flowerPerlin.SetFrequency(16);
		m_flowerPerlin.SetLacunarity(2.0);
		m_flowerPerlin.SetPersistence(0.5);
		m_flowerPerlin.SetOctaveCount(2);
		m_flowerPerlin.SetNoiseQuality(noise::QUALITY_FAST);
		
		for (noise::module::Perlin& cavePerlin : m_caveDirectionPerlin)
		{
			cavePerlin.SetFrequency(16);
			cavePerlin.SetLacunarity(2.0);
			cavePerlin.SetPersistence(0.8);
			cavePerlin.SetOctaveCount(1);
			cavePerlin.SetNoiseQuality(noise::QUALITY_FAST);
		}
		
		m_caveRadiusPerlin.SetFrequency(16);
		m_caveRadiusPerlin.SetLacunarity(2.0);
		m_caveRadiusPerlin.SetPersistence(0.8);
		m_caveRadiusPerlin.SetOctaveCount(1);
		m_caveRadiusPerlin.SetNoiseQuality(noise::QUALITY_FAST);
		
		SetSeed(0);
	}
	
	void WorldGenerator::SetSeed(int seed)
	{
		std::mt19937 rand(seed);
		
		m_heightPerlin.SetSeed(rand());
		m_flowerPerlin.SetSeed(rand());
		m_terracePerlin.SetSeed(rand());
		
		for (noise::module::Perlin& cavePerlin : m_caveDirectionPerlin)
		{
			cavePerlin.SetSeed(rand());
		}
		m_caveRadiusPerlin.SetSeed(rand());
	}
	
	WorldGenerator::FutureRegion& WorldGenerator::FindFutureRegion(RegionCoordinate coordinate)
	{
		auto it = std::find_if(MAKE_RANGE(m_futureRegions), [&] (const FutureRegion& region)
		{
			return region.m_coordinate == coordinate;
		});
		
		if (it != m_futureRegions.end())
			return *it;
		
		m_futureRegions.emplace_back();
		FutureRegion& result = m_futureRegions.back();
		result.m_coordinate = coordinate;
		return result;
	}
	
	//Average surface level Y-coordinate (in blocks).
	const int averageSurfaceLevel = 150;
	
	//Surface level range (in blocks) at minimum roughness.
	const double minSurfaceLevelRange = 10;
	
	//Surface level range (in blocks) at maximum roughness.
	const double maxSurfaceLevelRange = 25;
	
	//Average number of surface blocks with flowers as a percentage.
	const double flowerFrequency = 0.2;
	
	const uint8_t flowerIDs[] =
	{
		BlockIDs::BlueFlower, BlockIDs::RedFlower, BlockIDs::YellowFlower, BlockIDs::OrangeFlower, BlockIDs::WhiteFlower
	};
	
	std::uniform_real_distribution<double> caveWormPerlinPosDist(-1E6, 1E6);
	std::uniform_real_distribution<double> caveWormWorldPosXZDist(0, Region::Size);
	std::uniform_real_distribution<double> caveWormWorldPosYDist(0, averageSurfaceLevel);
	
	std::uniform_int_distribution<int> numCaveWormsDist(0, 2);
	
	//Minimum and maximum radius for caves (in blocks).
	const double caveMinRadius = 2.0;
	const double caveMaxRadius = 3.0;
	
	//Distribution for the length of caves (in blocks)
	std::uniform_int_distribution<int> caveWormLengthDist(150, 250);
	
	//The chance for a region to spawn a cave.
	const double caveGenerateProbability = 0.3;
	
	//Determines how quickly caves change direction.
	const double caveDirectionProgressRate = 0.01;
	
	//Determines how quickly caves change radius.
	const double caveRadiusProgressRate = 0.05;
	
	//Number of terraces.
	const int terraceCount = 2;
	
	//Height (in blocks) difference between each terrace.
	const double terraceHeight = 8;
	
	//Slope for the transition between terraces.
	const double terraceSlope = 10;
	
	//Progresses a single cave worm (and carves out blocks) until it escapes the region or reaches it's target length.
	void WorldGenerator::ProcessCaveWorm(WorldGenerator::CaveWorm worm, Region& region)
	{
		const int64_t regionMinX = region.GetX() * Region::Size;
		const int64_t regionMinZ = region.GetZ() * Region::Size;
		
		while (worm.m_distLeft > 0)
		{
			const glm::ivec3 regionPos(static_cast<int64_t>(std::floor(worm.m_worldPos.x)) - regionMinX,
			                           static_cast<int64_t>(std::floor(worm.m_worldPos.y)),
			                           static_cast<int64_t>(std::floor(worm.m_worldPos.z)) - regionMinZ);
			
			if (regionPos.y < 0 || regionPos.y > averageSurfaceLevel)
				break;
			
			if (regionPos.z < 0 || regionPos.x < 0 || regionPos.x >= Region::Size || regionPos.z >= Region::Size)
			{
				int64_t newRegX = std::floor(worm.m_worldPos.x / Region::Size);
				int64_t newRegZ = std::floor(worm.m_worldPos.z / Region::Size);
				
				std::lock_guard<std::mutex> futureRegionsLock(m_futureRegionsMutex);
				FindFutureRegion({ newRegX, newRegZ }).m_worms.push_back(worm);
				break;
			}
			
			const double radiusVal = m_caveRadiusPerlin.GetValue(worm.m_radPerlinPos.x, worm.m_radPerlinPos.y,
			                                                     worm.m_radPerlinPos.z);
			const double radiusD = glm::mix(caveMinRadius, caveMaxRadius, radiusVal * 0.5 + 0.5);
			const double radiusDSq = radiusD * radiusD;
			
			const int radiusI = std::ceil(radiusD);
			
			for (int yo = -radiusI; yo <= radiusI; yo++)
			{
				for (int xo = -radiusI; xo <= radiusI; xo++)
				{
					for (int zo = -radiusI; zo <= radiusI; zo++)
					{
						if (xo * xo + yo * yo + zo * zo > radiusDSq)
							continue;
						
						glm::ivec3 blockRegPos = regionPos + glm::ivec3(xo, yo, zo);
						if (blockRegPos.x >= 0 && blockRegPos.y >= 0 && blockRegPos.z >= 0 &&
						    blockRegPos.x < Region::Size && blockRegPos.y < Region::Height &&
						    blockRegPos.z < Region::Size)
						{
							region.At(blockRegPos.x, blockRegPos.y, blockRegPos.z).m_id = BlockIDs::Air;
						}
					}
				}
			}
			
			glm::dvec3 delta;
			for (int i = 0; i < 3; i++)
			{
				delta[i] = m_caveDirectionPerlin[i].GetValue(worm.m_dirPerlinPos.x, worm.m_dirPerlinPos.y, worm.m_dirPerlinPos.z);
			}
			worm.m_worldPos += glm::normalize(delta) * glm::dvec3(1.7, 0.7, 1.7);
			
			worm.m_distLeft--;
			
			worm.m_dirPerlinPos.x += caveDirectionProgressRate;
			worm.m_radPerlinPos.x += caveRadiusProgressRate;
		}
	}
	
	void WorldGenerator::ProcessFutureRegion(const WorldGenerator::FutureRegion& futureRegion, Region& region)
	{
		for (const CaveWorm& worm : futureRegion.m_worms)
		{
			ProcessCaveWorm(worm, region);
		}
		
		for (const FutureBlockPlacement& blockPlacement : futureRegion.m_blockPlacements)
		{
			region.At(blockPlacement.m_x, blockPlacement.m_y, blockPlacement.m_z) = blockPlacement.m_block;
		}
	}
	
	void WorldGenerator::Generate(Region& region)
	{
		std::subtract_with_carry_engine<uint64_t, 48, 5, 12> randEngine(region.GetX() ^ bswap_64(region.GetZ()));
		
		double PerlinDiv = 25;
		
		const int64_t regionMinX = region.GetX() * Region::Size;
		const int64_t regionMinZ = region.GetZ() * Region::Size;
		
		// ** Generates basic terrain **
		for (int lz = 0; lz < Region::Size; lz++)
		{
			int64_t z = lz + region.GetZ() * Region::Size;
			double pz = z / PerlinDiv;
			
			for (int lx = 0; lx < Region::Size; lx++)
			{
				int64_t x = lx + region.GetX() * Region::Size;
				double px = x / PerlinDiv;
				
				int blocksSinceAir = 0;
				
				double surfaceLevelRange = glm::mix(minSurfaceLevelRange, maxSurfaceLevelRange,
				                                    m_roughnessPerlin.GetValue(px, 0, pz) * 0.5 + 0.5);
				
				double terraceVal = (m_terracePerlin.GetValue(px, 0, pz) * 0.5 + 0.5) * terraceCount;
				const double heightCurrentTerrace = glm::clamp(terraceSlope * (glm::fract(terraceVal) - 0.5) + 0.5, 0.0, 1.0);
				
				double terraceOffset = (glm::floor(terraceVal) + heightCurrentTerrace - (terraceCount / 2.0)) * terraceHeight;
				
				for (int y = averageSurfaceLevel + maxSurfaceLevelRange; y >= 0; y--)
				{
					Region::BlockEntry& block = region.At(lx, y, lz);
					
					block.m_data = 0;
					
					double py = y / PerlinDiv;
					
					double heightVal = m_heightPerlin.GetValue(px, py, pz);
					heightVal += (y - (averageSurfaceLevel + terraceOffset)) / surfaceLevelRange;
					
					if (heightVal > 0)
					{
						blocksSinceAir = 0;
						block.m_id = BlockIDs::Air;
					}
					else
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
						
						blocksSinceAir++;
					}
				}
			}
		}
		
		// ** Potentially spawns a new cave in this region **
		if (std::uniform_real_distribution<double>()(randEngine) < caveGenerateProbability)
		{
			CaveWorm worm;
			
			worm.m_distLeft = caveWormLengthDist(randEngine);
			
			worm.m_worldPos = 
			{
				regionMinX + caveWormWorldPosXZDist(randEngine),
				caveWormWorldPosYDist(randEngine),
				regionMinZ + caveWormWorldPosXZDist(randEngine)
			};
			
			for (int j = 0; j < 3; j++)
			{
				worm.m_dirPerlinPos[j] = caveWormPerlinPosDist(randEngine);
			}
			
			ProcessCaveWorm(worm, region);
		}
		
		std::unique_lock<std::mutex> futureRegionsLock(m_futureRegionsMutex);
		
		auto futureRegionIt = std::find_if(MAKE_RANGE(m_futureRegions), [&] (const FutureRegion& futureRegion)
		{
			return futureRegion.m_coordinate.x == region.GetX() && futureRegion.m_coordinate.z == region.GetZ();
		});
		
		if (futureRegionIt != m_futureRegions.end())
		{
			FutureRegion futureRegion = std::move(*futureRegionIt);
			if (std::next(futureRegionIt) != m_futureRegions.end())
			{
				*futureRegionIt = std::move(m_futureRegions.back());
			}
			m_futureRegions.pop_back();
			
			futureRegionsLock.unlock();
			
			ProcessFutureRegion(futureRegion, region);
		}
	}
	
	void WorldGenerator::ProcessFutureRegions(WorldManager& worldManager)
	{
		std::unique_lock<std::mutex> lock(m_futureRegionsMutex);
		
		for (size_t i = 0; i < m_futureRegions.size();)
		{
			Region* region = worldManager.GetRegion(m_futureRegions[i].m_coordinate);
			
			if (region == nullptr)
			{
				i++;
				continue;
			}
			
			FutureRegion futureRegion = std::move(m_futureRegions[i]);
			
			if (i != m_futureRegions.size() - 1)
			{
				m_futureRegions[i] = std::move(m_futureRegions.back());
			}
			m_futureRegions.pop_back();
			
			lock.unlock();
			
			ProcessFutureRegion(futureRegion, *region);
			
			worldManager.MarkOutOfDate(futureRegion.m_coordinate);
			
			lock.lock();
		}
	}
}
