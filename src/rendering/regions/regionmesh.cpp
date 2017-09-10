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
		if (m_data.m_numIndices == 0)
		{
			return;
		}
		
		if (!m_hasAquiredDataBuffer)
		{
			uint64_t bufferSize = m_indicesOffset + m_data.m_numIndices * sizeof(uint32_t);
			
			//Aquires the device buffer from the transfer queue.
			const VkBufferMemoryBarrier barrier = 
			{
				/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				/* pNext               */ nullptr,
				/* srcAccessMask       */ 0,
				/* dstAccessMask       */ VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
				/* srcQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_TRANSFER],
				/* dstQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_GRAPHICS],
				/* buffer              */ m_data.m_buffer.GetBuffer(),
				/* offset              */ 0,
				/* size                */ bufferSize
			};
			commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
			                              { }, SingleElementSpan(barrier), { });
			
			m_hasAquiredDataBuffer = true;
		}
	}
	
	void RegionMesh::Render(CommandBuffer& commandBuffer) const
	{
		if (m_data.m_numIndices == 0)
		{
			return;
		}
		
		//TODO: Frustum culling of slices
		
		VkBuffer buffer = m_data.m_buffer.GetBuffer();
		VkDeviceSize verticesOffset = 0;
		commandBuffer.BindVertexBuffers(0, 1, &buffer, &verticesOffset);
		
		commandBuffer.BindIndexBuffer(buffer, m_indicesOffset, VK_INDEX_TYPE_UINT32);
		
		commandBuffer.DrawIndexed(m_data.m_numIndices, 1, 0, 0, 0);
	}
	
	
}
