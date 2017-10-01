#pragma once

#include "chunkbufferallocator.h"
#include "../../world/region.h"

namespace MCR
{
	class ChunkMesh
	{
	public:
		ChunkMesh() = default;
		
		ChunkMesh(uint64_t numIndices, uint64_t numVertices, Region::ChunkConnectivity connectivity);
		
		inline bool HasData() const
		{
			return m_allocation.HasData();
		}
		
		void Upload(CommandBuffer& commandBuffer, VkBuffer srcBuffer);
		
		void PrepareForRendering(CommandBuffer& commandBuffer)
		{
			m_allocation.PrepareForRendering(commandBuffer);
			m_allocation.MarkUsed();
		}
		
		inline void Reset()
		{
			m_allocation.Reset();
		}
		
		inline VkBuffer GetVertexBuffer() const
		{
			return m_allocation.GetVertexBuffer();
		}
		
		inline VkBuffer GetIndexBuffer() const
		{
			return m_allocation.GetIndexBuffer();
		}
		
		inline void WriteIndirectCommand(VkDrawIndexedIndirectCommand& command) const
		{
			command.vertexOffset = m_allocation.GetVertexOffset();
			command.firstIndex = m_allocation.GetIndexOffset();
			command.indexCount = m_allocation.GetNumIndices();
			command.instanceCount = 1;
			command.firstInstance = 0;
		}
		
		inline bool IsSideConnected(uint8_t side1, uint8_t side2) const
		{
			return m_connectivity.IsConnected(side1, side2);
		}
		
	private:
		ChunkBufferAllocator::Allocation m_allocation;
		Region::ChunkConnectivity m_connectivity;
	};
}
