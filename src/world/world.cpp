#include "world.h"
#include "../utils.h"

#include <zlib.h>
#include <cstring>

namespace MCR
{
	const uint16_t WorldVersion = 0;
	const char WorldMagic[] = { 'M', 'W' };
	
	constexpr uint8_t SectionID_Free = 0;
	constexpr uint8_t SectionID_RegionBegin = 1;
	constexpr uint8_t SectionID_RegionCont = 2;
	
#pragma pack(push, 1)
	struct FileHeader
	{
		char m_magic[sizeof(WorldMagic)];
		uint16_t m_version;
		uint64_t m_numSections;
	};
	
	struct RegionHeader
	{
		uint8_t m_identifier;
		int64_t m_x;
		int64_t m_z;
		uint16_t m_sectionCount;
		uint16_t m_lastSectionBytes;
		uint64_t m_nextSection;
	};
	
	struct RegionContHeader
	{
		uint8_t m_identifier;
		uint64_t m_nextSection;
	};
#pragma pack(pop)
	
	World::~World()
	{
		m_stream.seekp(offsetof(FileHeader, m_numSections), std::ios_base::beg);
		BinWrite<uint64_t>(m_stream, m_numSections);
		
		m_stream.close();
	}
	
	std::unique_ptr<World> World::Load(const fs::path& path)
	{
		std::unique_ptr<World> world(new World(path));
		
		FileHeader header;
		world->m_stream.read(reinterpret_cast<char*>(&header), sizeof(header));
		
		if (std::memcmp(header.m_magic, WorldMagic, sizeof(WorldMagic) != 0))
		{
			throw std::runtime_error("Invalid world file.");
		}
		
		if (header.m_version != WorldVersion)
		{
			throw std::runtime_error("Unsupported world version number.");
		}
		
		//Scans sections
		if (header.m_numSections != 0)
		{
			for (uint64_t i = 0;;)
			{
				RegionHeader regionHeader;
				world->m_stream.read(reinterpret_cast<char*>(&regionHeader), sizeof(regionHeader));
				
				switch (regionHeader.m_identifier)
				{
				case SectionID_RegionBegin:
					world->m_regions.push_back({ regionHeader.m_x, regionHeader.m_z, i });
					break;
				case SectionID_Free:
					world->m_availableSections.push_back(i);
					break;
				}
				
				i++;
				
				if (i == header.m_numSections)
					break;
				
				world->m_stream.seekp(SectionSize - sizeof(regionHeader), std::ios_base::cur);
			}
		}
		
		return world;
	}
	
	std::unique_ptr<World> World::CreateNew(const fs::path& path)
	{
		std::unique_ptr<World> world(new World(path, std::ios_base::trunc));
		
		FileHeader header;
		std::copy(MAKE_RANGE(WorldMagic), header.m_magic);
		header.m_version = WorldVersion;
		header.m_numSections = 0;
		
		world->m_stream.write(reinterpret_cast<const char*>(&header), sizeof(header));
		
		return world;
	}
	
	bool World::HasRegion(int64_t x, int64_t z)
	{
		std::lock_guard<std::mutex> lock(m_regionsMutex);
		
		auto regionIt = std::find_if(MAKE_RANGE(m_regions), [&] (const RegionEntry& region)
		{
			return region.m_x == x && region.m_z == z;
		});
		
		return regionIt != m_regions.end();
	}
	
	void World::LoadRegion(Region& region)
	{
		auto regionIt = std::find_if(MAKE_RANGE(m_regions), [&] (const RegionEntry& regionEntry)
		{
			return regionEntry.m_x == region.GetX() && regionEntry.m_z == region.GetZ();
		});
		
		if (regionIt == m_regions.end())
		{
			throw std::runtime_error("The region doesn't exist.");
		}
		
		SeekToSection(regionIt->m_firstSection);
		
		RegionHeader header;
		m_stream.read(reinterpret_cast<char*>(&header), sizeof(header));
		
		std::vector<char> inflatedRegionData(Region::DataBufferBytes);
		
		z_stream inflateStream = { };
		inflateInit(&inflateStream);
		
		inflateStream.avail_out = Region::DataBufferBytes;
		inflateStream.next_out = reinterpret_cast<Bytef*>(inflatedRegionData.data());
		
		//Reads and inflates sections
		for (uint16_t i = 0;;)
		{
			//Reads the continuation header
			RegionContHeader contHeader;
			if (i != 0)
			{
				m_stream.read(reinterpret_cast<char*>(&contHeader), sizeof(contHeader));
			}
			
			uint64_t sectionBytes;
			if (i + 1 == header.m_sectionCount)
			{
				sectionBytes = header.m_lastSectionBytes;
			}
			else if (i == 0)
			{
				sectionBytes = SectionSize - sizeof(RegionHeader);
			}
			else
			{
				sectionBytes = SectionSize - sizeof(RegionContHeader);
			}
			
			m_stream.read(m_ioBuffer, sectionBytes);
			
			inflateStream.next_in = reinterpret_cast<const Bytef*>(m_ioBuffer);
			inflateStream.avail_in = sectionBytes;
			
			int status = inflate(&inflateStream, Z_NO_FLUSH);
			
			switch (status)
			{
			case Z_MEM_ERROR:
				throw std::bad_alloc();
			case Z_STREAM_ERROR:
			case Z_NEED_DICT:
			case Z_DATA_ERROR:
				throw std::runtime_error("Invalid or corrupt region stream.");
			}
			
			i++;
			
			bool end = i == header.m_sectionCount;
			if ((status == Z_STREAM_END) != end)
			{
				//Stream ended at the wrong point.
				throw std::runtime_error("Invalid or corrupt region stream.");
			}
			
			if (end)
				break;
			
			SeekToSection(contHeader.m_nextSection);
		}
		
		region.Deserialize(inflatedRegionData.data());
	}
	
	void World::SaveRegion(const Region& region)
	{
		std::unique_lock<std::mutex> lock(m_regionsMutex);
		
		auto regionIt = std::find_if(MAKE_RANGE(m_regions), [&] (const RegionEntry& regionEntry)
		{
			return regionEntry.m_x == region.GetX() && regionEntry.m_z == region.GetZ();
		});
		
		//If this region already exists, marks it's sections as available.
		if (regionIt != m_regions.end())
		{
			m_availableSections.push_back(regionIt->m_firstSection);
			
			SeekToSection(regionIt->m_firstSection);
			
			RegionHeader header;
			m_stream.read(reinterpret_cast<char*>(&header), sizeof(header));
			
			uint64_t sectionIndex = header.m_nextSection;
			
			for (uint16_t i = 1; i < header.m_sectionCount; i++)
			{
				SeekToSection(sectionIndex);
				
				m_availableSections.push_back(sectionIndex);
				
				RegionContHeader contHeader;
				m_stream.read(reinterpret_cast<char*>(&contHeader), sizeof(contHeader));
				
				sectionIndex = contHeader.m_nextSection;
			}
		}
		else
		{
			m_regions.push_back({ region.GetX(), region.GetZ(), 0 });
			regionIt = std::prev(m_regions.end());
		}
		
		lock.unlock();
		
		auto AllocateSection = [&] ()
		{
			if (!m_availableSections.empty())
			{
				uint64_t sectionIndex = m_availableSections.back();
				m_availableSections.pop_back();
				return sectionIndex;
			}
			
			return m_numSections++;
		};
		
		std::vector<char> inflatedData(Region::DataBufferBytes);
		region.Serialize(inflatedData.data());
		
		z_stream deflateStream = { };
		deflateStream.avail_in = Region::DataBufferBytes;
		deflateStream.next_in = reinterpret_cast<const Bytef*>(inflatedData.data());
		if (deflateInit(&deflateStream, Z_DEFAULT_COMPRESSION) != Z_OK)
		{
			throw std::runtime_error("Error initializing ZLIB.");
		}
		
		uint64_t section = AllocateSection();
		regionIt->m_firstSection = section;
		
		SeekToSection(section);
		
		RegionHeader firstRegionHeader = { };
		firstRegionHeader.m_identifier = SectionID_RegionBegin;
		firstRegionHeader.m_x = region.GetX();
		firstRegionHeader.m_z = region.GetZ();
		m_stream.write(reinterpret_cast<const char*>(&firstRegionHeader), sizeof(firstRegionHeader));
		
		bool first = true;
		bool done = false;
		while (!done)
		{
			if (!first)
			{
				SeekToSection(section);
			}
			
			const size_t availSectionSpace = SectionSize - (first ? sizeof(RegionHeader) : sizeof(RegionContHeader));
			
			deflateStream.next_out = reinterpret_cast<Bytef*>(m_ioBuffer);
			deflateStream.avail_out = availSectionSpace;
			
			const int status = deflate(&deflateStream, Z_FINISH);
			
			done = (status == Z_STREAM_END);
			
			if (done)
			{
				firstRegionHeader.m_lastSectionBytes = availSectionSpace - deflateStream.avail_out;
				
				//Fills the rest of the io buffer with zeroes.
				std::fill_n(m_ioBuffer + firstRegionHeader.m_lastSectionBytes, deflateStream.avail_out, 0);
			}
			else
			{
				section = AllocateSection();
				
				if (first)
				{
					firstRegionHeader.m_nextSection = section;
				}
			}
			
			firstRegionHeader.m_sectionCount++;
			
			if (!first)
			{
				RegionContHeader header;
				header.m_identifier = SectionID_RegionCont;
				header.m_nextSection = section;
				m_stream.write(reinterpret_cast<const char*>(&header), sizeof(header));
			}
			else
			{
				first = false;
			}
			
			m_stream.write(m_ioBuffer, availSectionSpace);
		}
		
		SeekToSection(regionIt->m_firstSection);
		
		m_stream.write(reinterpret_cast<const char*>(&firstRegionHeader), sizeof(firstRegionHeader));
	}
	
	void World::SeekToSection(uint64_t sectionIndex)
	{
		m_stream.seekp(sizeof(FileHeader) + sectionIndex * SectionSize, std::ios_base::beg);
	}
}
