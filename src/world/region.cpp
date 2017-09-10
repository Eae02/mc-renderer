#include "region.h"

namespace MCR
{
	constexpr int Region::Size;
	constexpr int Region::Height;
	constexpr int Region::BlockCount;
	constexpr size_t Region::DataBufferBytes;
	
	void Region::SetHasData(bool hasData)
	{
		if (!this->HasData() && hasData)
		{
			m_blocks.resize(BlockCount);
		}
		else if (this->HasData() && !hasData)
		{
			m_blocks.clear();
			m_blocks.shrink_to_fit();
		}
	}
	
	void Region::Deserialize(const char* data)
	{
		SetHasData(true);
		
		const uint8_t* blockIds = reinterpret_cast<const uint8_t*>(data);
		const uint8_t* blockData = reinterpret_cast<const uint8_t*>(data) + BlockCount;
		
		for (size_t i = 0; i < BlockCount; i++)
		{
			m_blocks[i].m_id = blockIds[i];
			m_blocks[i].m_data = blockData[i];
		}
	}
	
	void Region::Serialize(char* data) const
	{
		uint8_t* blockIds = reinterpret_cast<uint8_t*>(data);
		uint8_t* blockData = reinterpret_cast<uint8_t*>(data) + BlockCount;
		
		for (size_t i = 0; i < BlockCount; i++)
		{
			blockIds[i] = m_blocks[i].m_id;
			blockData[i] = m_blocks[i].m_data;
		}
	}
}
