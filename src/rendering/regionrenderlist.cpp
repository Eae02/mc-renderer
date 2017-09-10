#include "regionrenderlist.h"
#include "regions/regionmesh.h"

namespace MCR
{
	void RegionRenderList::Begin()
	{
		m_meshes.clear();
	}
	
	void RegionRenderList::Add(RegionMesh& mesh)
	{
		m_meshes.push_back(&mesh);
	}
	
	void RegionRenderList::End(CommandBuffer& cb)
	{
		for (RegionMesh* mesh : m_meshes)
		{
			mesh->PrepareForRendering(cb);
		}
	}
	
	void RegionRenderList::Render(CommandBuffer& cb)
	{
		for (const RegionMesh* mesh : m_meshes)
		{
			mesh->Render(cb);
		}
	}
}
