#include "regionmesh.h"

namespace MCR
{
	
	RegionMesh::RegionMesh()
	{
		m_data.m_numIndices = 0;
		m_data.m_numVertices = 0;
	}
	
	void RegionMesh::PrepareForRendering(CommandBuffer& commandBuffer)
	{
		if (m_data.m_numIndices != 0)
		{
			m_data.m_buffer.PrepareForDraw(commandBuffer);
		}
	}
	
	void RegionMesh::Render(CommandBuffer& commandBuffer) const
	{
		if (m_data.m_numIndices != 0)
		{
			//TODO: Frustum culling of slices
			
			VkBuffer buffer = m_data.m_buffer.GetBuffer();
			VkDeviceSize verticesOffset = 0;
			commandBuffer.BindVertexBuffers(0, 1, &buffer, &verticesOffset);
			
			commandBuffer.BindIndexBuffer(buffer, m_indicesOffset, VK_INDEX_TYPE_UINT32);
			
			commandBuffer.DrawIndexed(m_data.m_numIndices, 1, 0, 0, 0);
		}
	}
	
	
}
