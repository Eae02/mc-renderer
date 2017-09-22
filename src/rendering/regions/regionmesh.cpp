#include "regionmesh.h"

namespace MCR
{
	
	RegionMesh::RegionMesh()
	{
		m_data.m_numIndices = 0;
		m_data.m_numVertices = 0;
	}
	
	void RegionMesh::SetData(RegionMesh::Data data)
	{
		m_data = std::move(data);
		m_indicesOffset = data.m_numVertices * sizeof(Vertex);
	}
	
	bool RegionMesh::IsSliceEmpty(uint32_t slice) const
	{
		const uint32_t indexOffset = m_data.m_sliceData[slice].m_indexOffset;
		if (slice == 0)
			return indexOffset == 0;
		return indexOffset == m_data.m_sliceData[slice - 1].m_indexOffset;
	}
	
	void RegionMesh::PrepareForRendering(CommandBuffer& commandBuffer)
	{
		if (m_data.m_numIndices != 0)
		{
			m_data.m_buffer.PrepareForDraw(commandBuffer);
		}
	}
	
	void RegionMesh::Render(CommandBuffer& commandBuffer, uint32_t firstSlice, uint32_t numSlices) const
	{
		VkBuffer buffer = m_data.m_buffer.GetBuffer();
		VkDeviceSize verticesOffset = 0;
		commandBuffer.BindVertexBuffers(0, 1, &buffer, &verticesOffset);
		
		commandBuffer.BindIndexBuffer(buffer, m_indicesOffset, VK_INDEX_TYPE_UINT32);
		
		uint32_t firstIndex = m_data.m_sliceData[firstSlice].m_indexOffset;
		
		uint32_t lastSlice = firstSlice + numSlices;
		uint32_t lastIndex = lastSlice >= (Region::SliceCount - 1) ? m_data.m_numIndices :
		                                                             m_data.m_sliceData[lastSlice + 1].m_indexOffset;
		
		if (lastIndex > firstIndex)
		{
			commandBuffer.DrawIndexed(lastIndex - firstIndex, 1, firstIndex, 0, 0);
		}
	}
}
