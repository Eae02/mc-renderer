#include "region.h"
#include "../blocks/blocktype.h"
#include "../blocks/ids.h"

#include <stack>
#include <algorithm>

namespace MCR
{
	constexpr int Region::Size;
	constexpr int Region::Height;
	constexpr int Region::BlockCount;
	constexpr size_t Region::DataBufferBytes;
	
	void Region::ReadChunk(uint32_t index, std::istream& stream)
	{
		const uint32_t blocksPerChunk = Size * Size * Size;
		stream.read(reinterpret_cast<char*>(&m_blocks[index * blocksPerChunk]),
		            blocksPerChunk * sizeof(BlockEntry));
	}
	
	void Region::WriteChunk(uint32_t index, std::ostream& stream) const
	{
		const uint32_t blocksPerChunk = Size * Size * Size;
		stream.write(reinterpret_cast<const char*>(&m_blocks[index * blocksPerChunk]),
		             blocksPerChunk * sizeof(BlockEntry));
	}
	
	bool Region::IsChunkAir(int y) const
	{
		if (m_opaquePerChunk[y] != 0)
			return false;
		
		const uint32_t blocksPerChunk = Size * Size * Size;
		uint32_t basePos = y * blocksPerChunk;
		return std::all_of(m_blocks.begin() + basePos, m_blocks.begin() + basePos + blocksPerChunk,
		                   [&] (const BlockEntry& block) { return block.m_id == BlockIDs::Air; });
	}
	
	void Region::Set(int locX, int locY, int locZ, Region::BlockEntry newEntry)
	{
		const size_t index = GetBlockIndex(locX, locY, locZ);
		
		if (BlockType::GetByID(m_blocks[index].m_id).IsOpaque())
		{
			m_opaquePerChunk[locY / Size]--;
		}
		
		if (BlockType::GetByID(newEntry.m_id).IsOpaque())
		{
			m_opaquePerChunk[locY / Size]++;
		}
		
		if (m_blocks[index].m_id == BlockIDs::Water)
		{
			m_waterPerChunk[locY / Size]--;
		}
		
		if (newEntry.m_id == BlockIDs::Water)
		{
			m_waterPerChunk[locY / Size]++;
		}
		
		m_blocks[index] = newEntry;
	}
	
	Region::ChunkConnectivity Region::CalculateConnectivity(uint32_t chunkIndex) const
	{
		ChunkConnectivity connectivity;
		
		constexpr size_t blocksPerChunk = Size * Size * Size;
		
		using Coord = glm::tvec3<uint8_t>;
		
		//Amount of (stack) memory to allocate for the coordinates stack.
		//Should be equal to the worst case space complexity for the depth first search.
		constexpr size_t maxCoordinates = blocksPerChunk;
		
		std::array<Coord, maxCoordinates> coordinatesStack;
		size_t coordinatesStackSize = 0;
		
		std::bitset<blocksPerChunk> blocksVisited;
		
		const uint64_t baseBlockIndex = chunkIndex * blocksPerChunk;
		
		auto GetChunkBlockIndex = [] (Coord coord)
		{
			return coord.x + (coord.z + coord.y * Size) * Size;
		};
		
		const uint8_t constCoordValues[6] = 
		{
			Size - 1, 0,
			Size - 1, 0,
			Size - 1, 0,
		};
		
		const Coord steps[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
		
		for (uint8_t side = 0; side < 6; side++)
		{
			const uint8_t constCoord = side / 2;
			const uint8_t varyCoord1 = (constCoord + 1) % 3;
			const uint8_t varyCoord2 = (constCoord + 2) % 3;
			
			for (uint8_t i = 0; i < Region::Size; i++)
			{
				for (uint8_t j = 0; j < Region::Size; j++)
				{
					Coord srcCoord;
					srcCoord[constCoord] = constCoordValues[side];
					srcCoord[varyCoord1] = i;
					srcCoord[varyCoord2] = j;
					
					auto MaybePushCoord = [&] (Coord coord)
					{
						size_t index = GetChunkBlockIndex(coord);
						
						if (blocksVisited[index] || BlockType::GetByID(m_blocks[baseBlockIndex + index].m_id).IsOpaque())
							return false;
						
						coordinatesStack[coordinatesStackSize++] = coord;
						blocksVisited.set(index);
						return true;
					};
					
					if (!MaybePushCoord(srcCoord))
						continue;
					
					bool touchedEdges[6];
					memset(touchedEdges, 0, sizeof(touchedEdges));
					
					while (coordinatesStackSize > 0)
					{
						//Pops a coordinate from the coordinates stack.
						const Coord coord = coordinatesStack[--coordinatesStackSize];
						
						//Scans through each dimension and checks if the current block lays at the edge of the section
						//in this dimension. If it does, the edge is marked in touchedEdges. If it doesn't, the
						//coordinate closer to the edge is pushed to the coordinates stack (if it is not opaque).
						for (int dim = 0; dim < 3; dim++)
						{
							if (coord[dim] == 0)
							{
								//We touched the negative edge for this dimension.
								touchedEdges[dim * 2 + 1] = true;
							}
							else
							{
								MaybePushCoord(coord - steps[dim]);
							}
							
							if (coord[dim] == Size - 1)
							{
								//We touched the positive edge for this dimension.
								touchedEdges[dim * 2] = true;
							}
							else
							{
								MaybePushCoord(coord + steps[dim]);
							}
						}
					}
					
					//Marks all conbinations of touched edges as connected.
					for (uint8_t s1 = 1; s1 < 6; s1++)
					{
						if (touchedEdges[s1])
						{
							for (uint8_t s2 = 0; s2 < s1; s2++)
							{
								if (touchedEdges[s2])
								{
									connectivity.SetConnected(s1, s2);
								}
							}
						}
					}
				}
			}
		}
		
		return connectivity;
	}
	
	const uint16_t connectionIndices[6][6] = 
	{
	    /*         +X  -X  +Y  -Y  +Z  -Z */
	    /* +X */ {  0,  1,  2,  3,  4,  5 },
		/* -X */ {  1,  0,  6,  7,  8,  9 },
		/* +Y */ {  2,  6,  0, 10, 11, 12 },
		/* -Y */ {  3,  7, 10,  0, 13, 14 },
		/* +Z */ {  4,  8, 11, 13,  0, 15 },
		/* -Z */ {  5,  9, 12, 14, 15,  0 }
	};
	
	inline uint16_t GetConnectionMask(uint8_t side1, uint8_t side2)
	{
		return static_cast<uint16_t>(1) << connectionIndices[side1][side2];
	}
	
	bool Region::ChunkConnectivity::IsConnected(uint8_t side1, uint8_t side2) const
	{
		return m_data & GetConnectionMask(side1, side2);
	}
	
	void Region::ChunkConnectivity::SetConnected(uint8_t side1, uint8_t side2)
	{
		m_data |= GetConnectionMask(side1, side2);
	}
}
