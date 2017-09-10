#pragma once

#include "../../vulkan/vk.h"
#include "../../world/region.h"
#include "meshbuilder.h"
#include "regiondatabuffer.h"

namespace MCR
{
	class RegionMesh
	{
	public:
		static constexpr uint64_t NumSlices = Region::Height / Region::Size;
		
		RegionMesh();
		
		struct Data
		{
			RegionDataBuffer m_buffer;
			uint32_t m_numVertices;
			uint32_t m_numIndices;
			std::array<uint32_t, NumSlices> m_sliceOffsets;
		};
		
		inline void SetData(Data data)
		{
			m_data = std::move(data);
			m_indicesOffset = data.m_numVertices * sizeof(Vertex);
			m_hasAquiredDataBuffer = false;
		}
		
		inline void Clear()
		{
			m_data.m_buffer = { };
			m_data.m_numIndices = 0;
		}
		
		inline bool HasData()
		{
			return m_data.m_numIndices > 0;
		}
		
		void PrepareForRendering(CommandBuffer& commandBuffer);
		
		void Render(CommandBuffer& commandBuffer) const;
		
	private:
		Data m_data;
		VkDeviceSize m_indicesOffset;
		
		bool m_hasAquiredDataBuffer;
	};
}
