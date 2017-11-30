#pragma once

#include <gsl/gsl>
#include <glm/glm.hpp>

#include "watervertex.h"
#include "../../vulkan/vk.h"
#include "../../vulkan/bufferpoolset.h"

namespace MCR
{
	class WaterMesh
	{
	public:
		inline WaterMesh()
		{
			Clear();
		}
		
		inline ~WaterMesh()
		{
			ReleaseVertices();
			ReleaseIndices();
		}
		
		inline WaterMesh(WaterMesh&& other)
		    : m_verticesAllocation(other.m_verticesAllocation), m_indicesAllocation(other.m_indicesAllocation),
		      m_numIndices(other.m_numIndices)
		{
			Clear();
		}
		
		inline WaterMesh& operator=(WaterMesh&& other)
		{
			ReleaseIndices();
			ReleaseVertices();
			
			m_verticesAllocation = other.m_verticesAllocation;
			m_indicesAllocation = other.m_indicesAllocation;
			m_numIndices = other.m_numIndices;
			other.Clear();
			
			return *this;
		}
		
		void Upload(CommandBuffer& cb, gsl::span<const WaterVertex> vertices, gsl::span<const uint16_t> indices);
		
		inline VkBuffer GetVertexBuffer() const
		{
			return m_verticesAllocation.m_buffer;
		}
		
		inline VkBuffer GetIndexBuffer() const
		{
			return m_indicesAllocation.m_buffer;
		}
		
		inline void WriteIndirectCommand(VkDrawIndexedIndirectCommand& command) const
		{
			command.vertexOffset = static_cast<int32_t>(m_verticesAllocation.m_firstElement);
			command.firstIndex = static_cast<uint32_t>(m_indicesAllocation.m_firstElement);
			command.indexCount = m_numIndices;
			command.instanceCount = 1;
			command.firstInstance = 0;
		}
		
		inline bool Empty() const
		{
			return m_numIndices == 0;
		}
		
		inline bool HasData() const
		{
			return m_numIndices != 0;
		}
		
		static void CreateBuffers();
		static void DestroyBuffers();
		
		static void ProcessFreeList();
		
		static const VkPipelineVertexInputStateCreateInfo s_vertexInputState;
		
		static constexpr float WaterHeight = 0.6f;
		
	private:
		void ReleaseVertices();
		void ReleaseIndices();
		
		inline void Clear()
		{
			m_numIndices = 0;
			m_verticesAllocation.m_elementCount = 0;
			m_indicesAllocation.m_elementCount = 0;
		}
		
		BufferPoolSet::Allocation m_verticesAllocation;
		BufferPoolSet::Allocation m_indicesAllocation;
		
		uint32_t m_numIndices;
	};
}
