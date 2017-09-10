#include "meshbuilder.h"
#include "../../utils.h"

namespace MCR
{
	void MeshBuilder::Reset()
	{
		m_vertices.clear();
		m_indices.clear();
	}
	
	void MeshBuilder::FillUploadBuffer(void* memory) const
	{
		Vertex* verticesOut = reinterpret_cast<Vertex*>(memory);
		uint32_t* indicesOut = reinterpret_cast<uint32_t*>(verticesOut + m_vertices.size());
		
		std::copy(MAKE_RANGE(m_vertices), verticesOut);
		std::copy(MAKE_RANGE(m_indices), indicesOut);
	}
}
