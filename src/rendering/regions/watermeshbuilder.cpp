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
}
