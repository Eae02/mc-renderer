#pragma once

#include "../vertex.h"

#include <memory>
#include <vector>

namespace MCR
{
	class MeshBuilder
	{
	public:
		MeshBuilder() = default;
		
		void Reset();
		
		inline void AddTriangle(uint32_t i1, uint32_t i2, uint32_t i3)
		{
			m_indices.push_back(i1);
			m_indices.push_back(i2);
			m_indices.push_back(i3);
		}
		
		inline bool Empty() const
		{
			return m_vertices.empty();
		}
		
		inline uint32_t GetNumIndices() const
		{
			return static_cast<uint32_t>(m_indices.size());
		}
		
		inline uint32_t GetNumVertices() const
		{
			return static_cast<uint32_t>(m_vertices.size());
		}
		
		inline uint32_t GetNextVertexIndex() const
		{
			return static_cast<uint32_t>(m_vertices.size());
		}
		
		inline void AddVertex(glm::vec3 position, glm::vec3 normal, glm::vec3 tangent,
		                      glm::vec2 texCoord, int albedoLayer, int normalLayer, float roughness)
		{
			m_vertices.emplace_back();
			m_vertices.back().m_position = position;
			m_vertices.back().m_roughness = roughness;
			m_vertices.back().m_normal = normal;
			m_vertices.back().m_tangent = tangent;
			m_vertices.back().m_texCoord = glm::vec4(texCoord, albedoLayer, normalLayer);
		}
		
		void FillUploadBuffer(void* memory) const;
		
		inline uint64_t GetRequiredBufferSize() const
		{
			return sizeof(Vertex) * m_vertices.size() + sizeof(uint32_t) * m_indices.size();
		}
		
	private:
		std::vector<Vertex> m_vertices;
		std::vector<uint32_t> m_indices;
	};
}
