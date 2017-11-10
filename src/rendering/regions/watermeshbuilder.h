#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <gsl/span>

namespace MCR
{
	class WaterMeshBuilder
	{
	public:
		WaterMeshBuilder();
		
		void Reset();
		
		void AddQuad(const glm::vec3* vertices);
		
		inline bool Empty() const
		{
			return m_indices.empty();
		}
		
		inline gsl::span<const glm::vec3> GetVertices() const
		{
			return m_vertices;
		}
		
		inline gsl::span<const uint16_t> GetIndices() const
		{
			return m_indices;
		}
		
	private:
		std::vector<glm::vec3> m_vertices;
		std::vector<uint16_t> m_indices;
	};
}
