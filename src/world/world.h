#pragma once

#include <fstream>
#include <optional>

#include "region.h"
#include "../filesystem.h"

namespace MCR
{
	/*
		HasRegion is internally synchronized and can be called async with LoadRegion and SaveRegion.
		LoadRegion and SaveRegion must only be called from one thread at a time.
	*/
	class World final
	{
	public:
		World(const fs::path& dirPath);
		
		void Save() const;
		
		bool HasRegion(int64_t x, int64_t z);
		
		void LoadRegion(Region& region);
		
		void SaveRegion(const Region& region);
		
	private:
		inline std::string GetRegionPath(int64_t x, int64_t z);
		
		fs::path m_path;
		
		std::mutex m_regionsMutex;
		std::vector<RegionCoordinate> m_regions;
		
		glm::vec3 m_cameraPosition;
		glm::vec2 m_cameraRotation;
	};
}
