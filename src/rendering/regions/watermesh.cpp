#include "watermesh.h"
#include <memory>

namespace MCR
{
	static const VkVertexInputBindingDescription waterVertexInputBinding =
	{
		/* binding   */ 0,
		/* stride    */ sizeof(float) * 3,
		/* inputRate */ VK_VERTEX_INPUT_RATE_VERTEX
	};
	
	const VkVertexInputAttributeDescription waterVertexAttributes[] =
	{
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 }
	};
	
	const VkPipelineVertexInputStateCreateInfo WaterMesh::s_vertexInputState = 
	{
		/* sType                           */ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		/* pNext                           */ nullptr,
		/* flags                           */ 0,
		/* vertexBindingDescriptionCount   */ 1,
		/* pVertexBindingDescriptions      */ &waterVertexInputBinding,
		/* vertexAttributeDescriptionCount */ static_cast<uint32_t>(ArrayLength(waterVertexAttributes)),
		/* pVertexAttributeDescriptions    */ waterVertexAttributes
	};
	
	const uint32_t VerticesPerPage = 4096 * 4096;
	const VkBufferUsageFlags VertexUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	const size_t VertexSize = sizeof(float) * 3;
	
	//Most vertices will belong to 4 triangles, so allocate space for 4 times as many indices as vertices.
	const uint32_t IndicesPerPage = VerticesPerPage * 4;
	const VkBufferUsageFlags IndexUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	
	static std::unique_ptr<BufferPoolSet> vertexPoolSet;
	static std::unique_ptr<BufferPoolSet> indexPoolSet;
	
	struct FreeListEntry
	{
		uint64_t m_targetFrame;
		bool m_indexAllocation;
		BufferPoolSet::Allocation m_allocation;
	};
	static std::vector<FreeListEntry> allocationsToFree;
	
	void WaterMesh::CreateBuffers()
	{
		vertexPoolSet = std::make_unique<BufferPoolSet>(VertexSize, VerticesPerPage, VertexUsage);
		indexPoolSet = std::make_unique<BufferPoolSet>(sizeof(uint16_t), IndicesPerPage, IndexUsage);
	}
	
	void WaterMesh::DestroyBuffers()
	{
		vertexPoolSet.reset();
		indexPoolSet.reset();
	}
	
	void WaterMesh::ProcessFreeList()
	{
		for (long i = static_cast<long>(allocationsToFree.size()) - 1; i >= 0; i--)
		{
			if (allocationsToFree[i].m_targetFrame > frameIndex)
				continue;
			
			BufferPoolSet* poolSet = allocationsToFree[i].m_indexAllocation ? indexPoolSet.get() : vertexPoolSet.get();
			poolSet->Free(allocationsToFree[i].m_allocation);
			
			allocationsToFree.back() = allocationsToFree[i];
			allocationsToFree.pop_back();
		}
	}
	
	void WaterMesh::Upload(CommandBuffer& cb, gsl::span<const glm::vec3> vertices, gsl::span<const uint16_t> indices)
	{
		const uint64_t numVertices = static_cast<size_t>(vertices.size());
		if (numVertices > m_verticesAllocation.m_elementCount)
		{
			ReleaseVertices();
			m_verticesAllocation = vertexPoolSet->Allocate(RoundToNextMultiple<size_t>(numVertices, 16));
		}
		
		const uint64_t numIndices = static_cast<size_t>(indices.size());
		if (numIndices > m_indicesAllocation.m_elementCount)
		{
			ReleaseIndices();
			m_indicesAllocation = indexPoolSet->Allocate(RoundToNextMultiple<size_t>(numIndices, 16));
		}
		
		m_numIndices = static_cast<uint32_t>(indices.size());
		
		VkDeviceSize verticesDstOffset = VertexSize * m_verticesAllocation.m_firstElement;
		VkDeviceSize indicesDstOffset = sizeof(uint16_t) * m_indicesAllocation.m_firstElement;
		
		cb.UpdateBuffer(m_verticesAllocation.m_buffer, verticesDstOffset,
		                static_cast<VkDeviceSize>(vertices.size_bytes()), vertices.data());
		
		cb.UpdateBuffer(m_indicesAllocation.m_buffer, indicesDstOffset,
		                static_cast<VkDeviceSize>(indices.size_bytes()), indices.data());
		
		VkBufferMemoryBarrier barriers[2];
		
		//Barrier for vertices
		barriers[0] =
		{
			/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			/* pNext               */ nullptr,
			/* srcAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
			/* dstAccessMask       */ VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
			/* srcQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
			/* dstQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
			/* buffer              */ m_verticesAllocation.m_buffer,
			/* offset              */ verticesDstOffset,
			/* size                */ static_cast<VkDeviceSize>(vertices.size_bytes())
		};
		
		//Barrier for indices
		barriers[1] =
		{
			/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			/* pNext               */ nullptr,
			/* srcAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
			/* dstAccessMask       */ VK_ACCESS_INDEX_READ_BIT,
			/* srcQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
			/* dstQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
			/* buffer              */ m_indicesAllocation.m_buffer,
			/* offset              */ indicesDstOffset,
			/* size                */ static_cast<VkDeviceSize>(indices.size_bytes())
		};
		
		cb.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, { }, barriers, { });
	}
	
	void WaterMesh::Draw(CommandBuffer& cb)
	{
		
	}
	
	void WaterMesh::ReleaseVertices()
	{
		if (m_verticesAllocation.m_elementCount != 0)
		{
			allocationsToFree.push_back({ frameIndex + SwapChain::GetImageCount(), false, m_verticesAllocation });
			m_verticesAllocation.m_elementCount = 0;
		}
	}
	
	void WaterMesh::ReleaseIndices()
	{
		if (m_indicesAllocation.m_elementCount != 0)
		{
			allocationsToFree.push_back({ frameIndex + SwapChain::GetImageCount(), true, m_indicesAllocation });
			m_indicesAllocation.m_elementCount = 0;
		}
	}
}
