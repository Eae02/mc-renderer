#include "regionrenderlist.h"
#include "regions/regionmesh.h"

namespace MCR
{
	void RegionRenderList::Begin()
	{
		m_entries.clear();
	}
	
	bool RegionRenderList::Add(RegionMesh& mesh, uint32_t slice)
	{
		const RenderEntry newEntry = { &mesh, slice };
		m_entries.push_back(newEntry);
		return true;
		
		auto it = std::lower_bound(MAKE_RANGE(m_entries), newEntry, [] (const RenderEntry& a, const RenderEntry& b)
		{
			return a.m_mesh < b.m_mesh || (a.m_mesh == b.m_mesh && a.m_slice < b.m_slice);
		});
		
		if (it != m_entries.end() && it->m_mesh == &mesh && it->m_slice == slice)
			return false;
		
		m_entries.insert(it, newEntry);
		return true;
	}
	
	void RegionRenderList::End(CommandBuffer& cb)
	{
		size_t i = 0;
		while (i < m_entries.size())
		{
			RegionMesh* mesh = m_entries[i].m_mesh;
			
			mesh->PrepareForRendering(cb);
			
			while (i < m_entries.size() && m_entries[i].m_mesh == mesh)
			{
				i++;
			}
		}
	}
	
	void RegionRenderList::Render(CommandBuffer& cb)
	{
		if (m_entries.empty())
			return;
		
		const RegionMesh* mesh = m_entries[0].m_mesh;
		uint32_t firstSlice = m_entries[0].m_slice;
		uint32_t numSlices = 1;
		
		size_t i = 1;
		while (true)
		{
			bool end = i == m_entries.size();
			if (end || mesh != m_entries[i].m_mesh || m_entries[i].m_slice != (firstSlice + numSlices))
			{
				mesh->Render(cb, firstSlice, numSlices);
				
				mesh = m_entries[i].m_mesh;
				firstSlice = m_entries[i].m_slice;
				numSlices = 0;
			}
			
			if (end)
				break;
			
			numSlices++;
			i++;
		}
	}
}
