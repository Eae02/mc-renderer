#pragma once

#include <cstdint>
#include <memory>
#include <bitset>
#include "../vulkan/vk.h"

namespace MCR
{
	struct RegionCoordinate
	{
		int64_t x;
		int64_t z;
		
		inline bool operator==(RegionCoordinate other) const
		{
			return x == other.x && z == other.z;
		}
		
		inline bool operator!=(RegionCoordinate other) const
		{
			return x != other.x || z != other.z;
		}
		
		inline static uint64_t DistanceSq(const RegionCoordinate& a, const RegionCoordinate& b)
		{
			int64_t dx = a.x - b.x;
			int64_t dz = a.z - b.z;
			return static_cast<uint64_t>(dx * dx) + static_cast<uint64_t>(dz * dz);
		}
	};
	
	//Moving is very slow!
	class Region
	{
	public:
		struct BlockEntry
		{
			uint8_t m_id = 0;
			uint8_t m_data = 0;
		};
		
		class SliceConnectivity
		{
			friend class Region;
			
		public:
			bool IsConnected(uint8_t side1, uint8_t side2) const;
			
		private:
			void SetConnected(uint8_t side1, uint8_t side2);
			
			uint16_t m_data = 0x1;
		};
		
		Region() = default;
		
		inline Region(int64_t coordX, int64_t coordZ)
		    : m_coordX(coordX), m_coordZ(coordZ) { }
		
		inline void SetPosition(int64_t coordX, int64_t coordZ)
		{
			m_coordX = coordX;
			m_coordZ = coordZ;
		}
		
		inline BlockEntry& At(int locX, int locY, int locZ)
		{
			return m_blocks[GetBlockIndex(locX, locY, locZ)];
		}
		
		inline BlockEntry At(int locX, int locY, int locZ) const
		{
			return m_blocks[GetBlockIndex(locX, locY, locZ)];
		}
		
		inline int64_t GetX() const
		{ return m_coordX; }
		inline int64_t GetZ() const
		{ return m_coordZ; }
		
		SliceConnectivity CalculateConnectivity(uint32_t sliceIndex) const;
		
		void Deserialize(const char* data);
		void Serialize(char* data) const;
		
		static constexpr int Size = 16;
		static constexpr int Height = 256;
		static constexpr int SliceCount = Height / Size;
		static constexpr int BlockCount = Size * Size * Height;
		static constexpr size_t DataBufferBytes = BlockCount * (sizeof(uint8_t) + sizeof(uint8_t));
		
	private:
		inline static size_t GetBlockIndex(int locX, int locY, int locZ)
		{
#ifdef MCR_DEBUG
			if (locX < 0 || locY < 0 || locZ < 0 || locX >= static_cast<int>(Size) ||
			    locY >= static_cast<int>(Height) || locZ >= static_cast<int>(Size))
			{
				throw std::out_of_range("Chunk index out of range");
			}
#endif
			return locX + (locZ + locY * Size) * Size;
		}
		
		std::array<BlockEntry, BlockCount> m_blocks;
		
		int64_t m_coordX;
		int64_t m_coordZ;
	};
}
