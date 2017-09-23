#include "chunkmesh.h"
#include "../vertex.h"

namespace MCR
{
	ChunkMesh::ChunkMesh(uint64_t numIndices, uint64_t numVertices, Region::ChunkConnectivity connectivity)
	    : m_allocation(ChunkBufferAllocator::s_instance.Allocate(numIndices, numVertices)),
	      m_connectivity(connectivity)
	{
		
	}
	
	void ChunkMesh::Upload(CommandBuffer& commandBuffer, VkBuffer srcBuffer)
	{
		m_allocation.BeforeTransfer(commandBuffer);
		
		const uint64_t verticesBytes = sizeof(Vertex) * m_allocation.GetNumVertices();
		const uint64_t indicesBytes = sizeof(uint32_t) * m_allocation.GetNumIndices();
		
		const VkBufferCopy vertexBufferCopy = { 0, m_allocation.GetVertexOffset() * sizeof(Vertex), verticesBytes };
		commandBuffer.CopyBuffer(srcBuffer, m_allocation.GetVertexBuffer(), vertexBufferCopy);
		
		const VkBufferCopy indexBufferCopy = { verticesBytes, m_allocation.GetIndexOffset() * sizeof(uint32_t), indicesBytes };
		commandBuffer.CopyBuffer(srcBuffer, m_allocation.GetIndexBuffer(), indexBufferCopy);
		
		m_allocation.AfterTransfer(commandBuffer);
	}
}
