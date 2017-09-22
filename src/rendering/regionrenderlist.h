#pragma once

#include "../vulkan/vk.h"

namespace MCR
{
	class RegionRenderList
	{
	public:
		RegionRenderList() = default;
		
		void Begin();
		
		bool Add(class RegionMesh& mesh, uint32_t slice);
		
		void End(CommandBuffer& cb);
		
		void Render(CommandBuffer& cb);
		
	private:
		struct RenderEntry
		{
			class RegionMesh* m_mesh;
			uint32_t m_slice;
		};
		
		std::vector<RenderEntry> m_entries;
	};
}
