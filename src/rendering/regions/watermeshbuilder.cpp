#include "watermeshbuilder.h"
#include "../../utils.h"

#include <algorithm>

namespace MCR
{
	WaterMeshBuilder::WaterMeshBuilder()
	{
		m_vertices.reserve(16);
		m_indices.reserve(16 * 4);
	}
	
	void WaterMeshBuilder::Reset()
	{
		m_vertices.clear();
		m_indices.clear();
	}
	
	void WaterMeshBuilder::AddQuad(const glm::vec3* vertices)
	{
		uint16_t indices[4];
		
		//Searches for the vertices in the vertices vector.
		for (int i = 0; i < 4; i++)
		{
			//auto vertexIt = std::find_if(MAKE_RANGE(m_vertices), [&] (const glm::vec3& vertex)
			//{
			//	return glm::distance(vertex.x, vertices[i].x) < 0.01f &&
			//	       glm::distance(vertex.y, vertices[i].y) < 0.01f &&
			//	       glm::distance(vertex.z, vertices[i].z) < 0.01f;
			//});
			//
			//if (vertexIt == m_vertices.end())
			//{
				indices[i] = static_cast<uint16_t>(m_vertices.size());
				m_vertices.push_back(vertices[i]);
			//}
			//else
			//{
			//	indices[i] = static_cast<uint16_t>(vertexIt - m_vertices.begin());
			//}
		}
		
		const size_t indicesOutPos = m_indices.size();
		m_indices.resize(m_indices.size() + 6);
		uint16_t* indicesOut = &m_indices[indicesOutPos];
		
		indicesOut[0] = indices[0];
		indicesOut[1] = indices[1];
		indicesOut[2] = indices[2];
		indicesOut[3] = indices[3];
		indicesOut[4] = indices[2];
		indicesOut[5] = indices[1];
	}
}
