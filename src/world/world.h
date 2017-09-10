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
		~World();
		
		static std::unique_ptr<World> Load(const fs::path& path);
		static std::unique_ptr<World> CreateNew(const fs::path& path);
		
		bool HasRegion(int64_t x, int64_t z);
		
		Region LoadRegion(int64_t x, int64_t z);
		
		void SaveRegion(const Region& region);
		
	private:
		static constexpr uint64_t SectionSize = 2048;
		
		explicit inline World(const fs::path& path, std::ios_base::openmode openMode = { })
		    : m_stream(path, std::ios_base::in | std::ios_base::out | std::ios_base::binary | openMode),
		      m_numSections(0) { }
		
		char m_ioBuffer[SectionSize];
		
		void SeekToSection(uint64_t sectionIndex);
		
		std::fstream m_stream;
		
		struct RegionEntry
		{
			int64_t m_x;
			int64_t m_z;
			uint64_t m_firstSection;
		};
		
		std::mutex m_regionsMutex;
		std::vector<RegionEntry> m_regions;
		
		std::vector<uint64_t> m_availableSections;
		size_t m_numSections;
	};
}
