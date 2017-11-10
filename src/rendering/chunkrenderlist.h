#pragma once

#include "../vulkan/vk.h"
#include "regions/chunkmesh.h"
#include "regions/watermesh.h"

namespace MCR
{
	class ChunkRenderList
	{
	public:
		ChunkRenderList() = default;
		
		void Begin();
		
		void Add(ChunkMesh& mesh);
		void Add(WaterMesh& mesh);
		
		void End(CommandBuffer& cb);
		
		void Render(CommandBuffer& cb) const;
		void RenderWater(CommandBuffer& cb) const;
		
	private:
		struct MeshGroup
		{
			VkBuffer m_vertexBuffer;
			VkBuffer m_indexBuffer;
			std::vector<ChunkMesh*> m_meshes;
			
			inline explicit MeshGroup(ChunkMesh& mesh)
			    : m_vertexBuffer(mesh.GetVertexBuffer()), m_indexBuffer(mesh.GetIndexBuffer()), m_meshes{ &mesh } { }
		};
		
		std::vector<MeshGroup> m_meshGroups;
		
		struct WaterMeshGroup
		{
			VkBuffer m_vertexBuffer;
			VkBuffer m_indexBuffer;
			std::vector<WaterMesh*> m_meshes;
			
			inline explicit WaterMeshGroup(WaterMesh& mesh)
				: m_vertexBuffer(mesh.GetVertexBuffer()), m_indexBuffer(mesh.GetIndexBuffer()), m_meshes{ &mesh } { }
		};
		
		std::vector<WaterMeshGroup> m_waterMeshGroups;
		
		uint32_t m_requiredIndirectCommands = 0;
		uint32_t m_numAllocatedCommands = 0;
		
		uint64_t m_waterIndirectCommandsOffset;
		
		VkHandle<VmaAllocation, VkHandleDestroyTime::Delayed> m_hostCommandsAllocation;
		VkHandle<VkBuffer, VkHandleDestroyTime::Delayed> m_hostCommandsBuffer;
		VkDrawIndexedIndirectCommand* m_hostCommandsMemory;
		
		VkHandle<VmaAllocation, VkHandleDestroyTime::Delayed> m_deviceCommandsAllocation;
		VkHandle<VkBuffer, VkHandleDestroyTime::Delayed> m_deviceCommandsBuffer;
	};
}
