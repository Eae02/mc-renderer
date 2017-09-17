#pragma once

#include <libnoise/module/perlin.h>
#include <libnoise/module/ridgedmulti.h>
#include <functional>

#include "region.h"

namespace MCR
{
	class WorldGenerator
	{
	public:
		WorldGenerator();
		
		void SetSeed(int seed);
		
		Region Generate(int64_t rx, int64_t rz);
		
		void ProcessFutureRegions(class WorldManager& worldManager);
		
	private:
		struct CaveWorm
		{
			glm::dvec3 m_dirPerlinPos;
			glm::dvec3 m_radPerlinPos;
			glm::dvec3 m_worldPos;
			int m_distLeft;
		};
		
		struct FutureBlockPlacement
		{
			uint8_t m_x;
			uint8_t m_y;
			uint8_t m_z;
			Region::BlockEntry m_block;
		};
		
		struct FutureRegion
		{
			RegionCoordinate m_coordinate;
			std::vector<CaveWorm> m_worms;
			std::vector<FutureBlockPlacement> m_blockPlacements;
		};
		
		void ProcessCaveWorm(CaveWorm worm, Region& region);
		
		void ProcessFutureRegion(const FutureRegion& futureRegion, Region& region);
		
		FutureRegion& FindFutureRegion(RegionCoordinate coordinate);
		
		std::mutex m_futureRegionsMutex;
		std::vector<FutureRegion> m_futureRegions;
		
		noise::module::Perlin m_roughnessPerlin;
		noise::module::Perlin m_heightPerlin;
		noise::module::Perlin m_terracePerlin;
		noise::module::Perlin m_flowerPerlin;
		noise::module::Perlin m_caveDirectionPerlin[3];
		noise::module::Perlin m_caveRadiusPerlin;
	};
}
