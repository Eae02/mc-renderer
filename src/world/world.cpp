#include "world.h"
#include "../utils.h"

#include <zlib.h>
#include <cstring>
#include <sstream>
#include <chrono>
#include <algorithm>

namespace MCR
{
	const uint16_t WorldVersion = 1;
	const char WorldMagic[] = { 'M', 'W' };
	const char RegionMagic[] = { 'M', 'R' };
	
#pragma pack(push, 1)
	struct WorldHeader
	{
		char m_magic[sizeof(WorldMagic)];
		uint16_t m_version;
		glm::vec3 m_cameraPosition;
		glm::vec2 m_cameraRotation;
	};
#pragma pack(pop)
	
	World::World(const fs::path& dirPath)
	    : m_path(dirPath)
	{
		std::ifstream worldStream(dirPath / "world", std::ios::binary);
		
		if (worldStream)
		{
			WorldHeader header;
			worldStream.read(reinterpret_cast<char*>(&header), sizeof(header));
			
			if (std::memcmp(header.m_magic, WorldMagic, sizeof(WorldMagic)) != 0)
			{
				throw std::runtime_error("Invalid world file.");
			}
			
			if (header.m_version != WorldVersion)
			{
				throw std::runtime_error("Unsupported world version number.");
			}
			
			m_cameraPosition = header.m_cameraPosition;
			m_cameraRotation = header.m_cameraRotation;
		}
	}
	
	void World::Save() const
	{
		WorldHeader header;
		std::copy(MAKE_RANGE(WorldMagic), header.m_magic);
		header.m_version = WorldVersion;
		header.m_cameraPosition = m_cameraPosition;
		header.m_cameraRotation = m_cameraRotation;
		
		std::ofstream worldStream(m_path / "world", std::ios::binary);
		worldStream.write(reinterpret_cast<const char*>(&header), sizeof(header));
	}
	
	bool World::HasRegion(int64_t x, int64_t z)
	{
		std::lock_guard<std::mutex> lock(m_regionsMutex);
		
		auto regionIt = std::find_if(MAKE_RANGE(m_regions), [&] (const RegionCoordinate& region)
		{
			return region.x == x && region.z == z;
		});
		
		return regionIt != m_regions.end();
	}
	
	std::string World::GetRegionPath(int64_t x, int64_t z)
	{
		std::ostringstream nameStream;
		nameStream << m_path.string() << "/" << x << "_" << z;
		return nameStream.str();
	}
	
	void World::LoadRegion(Region& region)
	{
		std::ifstream stream(GetRegionPath(region.GetX(), region.GetZ()), std::ios::binary);
		
		char magic[sizeof(RegionMagic)];
		stream.read(magic, sizeof(magic));
		if (std::memcmp(magic, RegionMagic, sizeof(RegionMagic)) != 0)
		{
			throw std::runtime_error("Invalid region file.");
		}
		
		uint8_t chunkCount;
		stream.read(reinterpret_cast<char*>(&chunkCount), sizeof(chunkCount));
		
		if (chunkCount != 0)
		{
			uint8_t* chunkIndices = reinterpret_cast<uint8_t*>(alloca(chunkCount));
			stream.read(reinterpret_cast<char*>(chunkIndices), chunkCount * sizeof(uint8_t));
			
			for (uint8_t i = 0; i < chunkCount; i++)
			{
				region.ReadChunk(chunkIndices[i], stream);
			}
		}
	}
	
	void World::SaveRegion(const Region& region)
	{
		std::ofstream stream(GetRegionPath(region.GetX(), region.GetZ()), std::ios::binary);
		
		stream.write(RegionMagic, sizeof(RegionMagic));
		
		uint8_t chunkIndices[Region::ChunkCount];
		uint8_t numChunkIndices = 0;
		
		for (uint8_t i = 0; i < Region::ChunkCount; i++)
		{
			if (region.IsChunkAir(i))
				continue;
			chunkIndices[numChunkIndices++] = i;
		}
		
		stream.write(reinterpret_cast<const char*>(&numChunkIndices), sizeof(numChunkIndices));
		stream.write(reinterpret_cast<const char*>(chunkIndices), numChunkIndices * sizeof(uint8_t));
		
		for (uint8_t i = 0; i < numChunkIndices; i++)
		{
			region.WriteChunk(chunkIndices[i], stream);
		}
	}
}
