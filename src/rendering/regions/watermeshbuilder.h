#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <gsl/span>

#include "watervertex.h"

namespace MCR
{
	class WaterMeshBuilder
	{
	public:
		WaterMeshBuilder();
		
		void Reset();
		
		inline void AddVertex(const WaterVertex& vertex)
		{
			m_vertices.push_back(vertex);
		}
		
		inline void AddTriangle(uint16_t v1, uint16_t v2, uint16_t v3)
		{
			m_indices.push_back(v1);
			m_indices.push_back(v2);
			m_indices.push_back(v3);
		}
		
		inline uint16_t GetNextIndex() const
		{
			return static_cast<uint16_t>(m_vertices.size());
		}
		
		inline bool Empty() const
		{
			return m_indices.empty();
		}
		
		inline gsl::span<const WaterVertex> GetVertices() const
		{
			return m_vertices;
		}
		
		inline gsl::span<const uint16_t> GetIndices() const
		{
			return m_indices;
		}
		
	private:
		std::vector<WaterVertex> m_vertices;
		std::vector<uint16_t> m_indices;
	};
}
