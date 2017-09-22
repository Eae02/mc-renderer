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
		
		struct SliceData
		{
			uint32_t m_indexOffset;
			Region::SliceConnectivity m_connectivity;
		};
		
		struct Data
		{
			RegionDataBuffer m_buffer;
			uint32_t m_numVertices;
			uint32_t m_numIndices;
			std::array<SliceData, NumSlices> m_sliceData;
		};
		
		void SetData(Data data);
		
		inline bool IsSliceEdgeConnected(uint32_t slice, uint8_t s1, uint8_t s2) const
		{
			return m_data.m_sliceData[slice].m_connectivity.IsConnected(s1, s2);
		}
		
		bool IsSliceEmpty(uint32_t slice) const;
		
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
		
		void Render(CommandBuffer& commandBuffer, uint32_t firstSlice, uint32_t numSlices) const;
		
	private:
		Data m_data;
		VkDeviceSize m_indicesOffset;
	};
}
