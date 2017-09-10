#pragma once

#include "../vulkan/vk.h"

namespace MCR
{
	class RegionRenderList
	{
	public:
		RegionRenderList() = default;
		
		void Begin();
		
		void Add(class RegionMesh& mesh);
		
		void End(CommandBuffer& cb);
		
		void Render(CommandBuffer& cb);
		
	private:
		std::vector<class RegionMesh*> m_meshes;
	};
}
