#include "chunkrenderlist.h"

#include <gsl/gsl>

namespace MCR
{
	void ChunkRenderList::Begin()
	{
		for (MeshGroup& group : m_meshGroups)
		{
			group.m_meshes.clear();
		}
		
		for (WaterMeshGroup& group : m_waterMeshGroups)
		{
			group.m_meshes.clear();
		}
		
		m_requiredIndirectCommands = 0;
	}
	
	void ChunkRenderList::Add(ChunkMesh& mesh)
	{
		auto groupIt = std::find_if(MAKE_RANGE(m_meshGroups), [&] (const MeshGroup& group)
		{
			return group.m_vertexBuffer == mesh.GetVertexBuffer() && group.m_indexBuffer == mesh.GetIndexBuffer();
		});
		
		if (groupIt != m_meshGroups.end())
		{
#ifdef MCR_DEBUG
			if(std::find(MAKE_RANGE(groupIt->m_meshes), &mesh) != groupIt->m_meshes.end())
			{
				throw std::runtime_error("ChunkRenderList: mesh added multiple times.");
			}
#endif
			
			groupIt->m_meshes.push_back(&mesh);
		}
		else
		{
			m_meshGroups.emplace_back(mesh);
		}
		
		m_requiredIndirectCommands++;
	}
	
	void ChunkRenderList::Add(WaterMesh& mesh)
	{
		auto groupIt = std::find_if(MAKE_RANGE(m_waterMeshGroups), [&] (const WaterMeshGroup& group)
		{
			return group.m_vertexBuffer == mesh.GetVertexBuffer() && group.m_indexBuffer == mesh.GetIndexBuffer();
		});
		
		if (groupIt != m_waterMeshGroups.end())
		{
#ifdef MCR_DEBUG
			if(std::find(MAKE_RANGE(groupIt->m_meshes), &mesh) != groupIt->m_meshes.end())
			{
				throw std::runtime_error("ChunkRenderList: water mesh added multiple times.");
			}
#endif
			
			groupIt->m_meshes.push_back(&mesh);
		}
		else
		{
			m_waterMeshGroups.emplace_back(mesh);
		}
		
		m_requiredIndirectCommands++;
	}
	
	void ChunkRenderList::End(CommandBuffer& cb)
	{
		if (m_requiredIndirectCommands == 0)
			return;
		
		if (!vulkan.limits.hasMultiDrawIndirect)
		{
			for (MeshGroup& group : m_meshGroups)
			{
				for (ChunkMesh* mesh : group.m_meshes)
				{
					mesh->PrepareForRendering(cb);
				}
			}
			
			return;
		}
		
		if (m_numAllocatedCommands < m_requiredIndirectCommands)
		{
			m_numAllocatedCommands = RoundToNextMultiple<uint32_t>(m_requiredIndirectCommands, 1024);
			
			const uint64_t bufferSize = m_numAllocatedCommands * sizeof(VkDrawIndexedIndirectCommand);
			
			// ** Allocates the host buffer **
			const VmaAllocationCreateInfo hostBufferAllocationCI =
			{
				VMA_ALLOCATION_CREATE_PERSISTENT_MAP_BIT,
				VMA_MEMORY_USAGE_CPU_ONLY
			};
			
			VkBufferCreateInfo hostBufferCreateInfo;
			InitBufferCreateInfo(hostBufferCreateInfo, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			                     bufferSize * SwapChain::GetImageCount());
			
			VmaAllocationInfo hostAllocationInfo;
			
			CheckResult(vmaCreateBuffer(vulkan.allocator, &hostBufferCreateInfo, &hostBufferAllocationCI,
			                            m_hostCommandsBuffer.GetCreateAddress(),
			                            m_hostCommandsAllocation.GetCreateAddress(), &hostAllocationInfo));
			
			m_hostCommandsMemory = reinterpret_cast<VkDrawIndexedIndirectCommand*>(hostAllocationInfo.pMappedData);
			
			// ** Allocates the device buffer **
			const VmaAllocationCreateInfo deviceBufferAllocationCI = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
			
			VkBufferCreateInfo deviceBufferCreateInfo;
			InitBufferCreateInfo(deviceBufferCreateInfo, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
			                     VK_BUFFER_USAGE_TRANSFER_DST_BIT, bufferSize);
			
			CheckResult(vmaCreateBuffer(vulkan.allocator, &deviceBufferCreateInfo, &deviceBufferAllocationCI,
			                            m_deviceCommandsBuffer.GetCreateAddress(),
			                            m_deviceCommandsAllocation.GetCreateAddress(), nullptr));
		}
		
		const uint64_t hostCommandsOffset = m_numAllocatedCommands * frameQueueIndex;
		VkDrawIndexedIndirectCommand* hostCommandsBegin = m_hostCommandsMemory + hostCommandsOffset;
		VkDrawIndexedIndirectCommand* nextHostCommand = hostCommandsBegin;
		
		for (MeshGroup& group : m_meshGroups)
		{
			for (ChunkMesh* mesh : group.m_meshes)
			{
				mesh->PrepareForRendering(cb);
				mesh->WriteIndirectCommand(*(nextHostCommand++));
			}
		}
		
		m_waterIndirectCommandsOffset = nextHostCommand - hostCommandsBegin;
		
		for (WaterMeshGroup& group : m_waterMeshGroups)
		{
			for (WaterMesh* mesh : group.m_meshes)
			{
				mesh->WriteIndirectCommand(*(nextHostCommand++));
			}
		}
		
		// ** Uploads indirect commands. **
		const VkBufferCopy commandsBufferCopy =
		{
			/* srcOffset */ hostCommandsOffset * sizeof(VkDrawIndexedIndirectCommand),
			/* dstOffset */ 0,
			/* size      */ m_requiredIndirectCommands * sizeof(VkDrawIndexedIndirectCommand)
		};
		cb.CopyBuffer(*m_hostCommandsBuffer, *m_deviceCommandsBuffer, commandsBufferCopy);
		
		// ** Inserts a barrier for indirect commands. **
		VkBufferMemoryBarrier commandsBufferBarrier = 
		{
			/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			/* pNext               */ nullptr,
			/* srcAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
			/* dstAccessMask       */ VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
			/* srcQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
			/* dstQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
			/* buffer              */ *m_deviceCommandsBuffer,
			/* offset              */ 0,
			/* size                */ commandsBufferCopy.size
		};
		
		cb.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0,
		                   { }, SingleElementSpan(commandsBufferBarrier), { });
	}
	
	template <typename T>
	inline void DrawMeshGroup(CommandBuffer& cb, const T& group, VkBuffer indirectCommandsBuffer, uint64_t& offset, 
	                          VkIndexType indexType)
	{
		cb.BindIndexBuffer(group.m_indexBuffer, 0, indexType);
		
		VkDeviceSize vbOffsets[] = { 0 };
		cb.BindVertexBuffers(0, 1, &group.m_vertexBuffer, vbOffsets);
		
		if (vulkan.limits.hasMultiDrawIndirect)
		{
			cb.DrawIndexedIndirect(indirectCommandsBuffer, offset, gsl::narrow<uint32_t>(group.m_meshes.size()),
			                       sizeof(VkDrawIndexedIndirectCommand));
			
			offset += group.m_meshes.size() * sizeof(VkDrawIndexedIndirectCommand);
		}
		else
		{
			VkDrawIndexedIndirectCommand command;
			
			for (const auto* mesh : group.m_meshes)
			{
				mesh->WriteIndirectCommand(command);
				
				cb.DrawIndexed(command.indexCount, command.instanceCount, command.firstIndex,
				               command.vertexOffset, command.firstInstance);
			}
		}
	}
	
	void ChunkRenderList::Render(CommandBuffer& cb) const
	{
		uint64_t offset = 0;
		
		for (const MeshGroup& group : m_meshGroups)
		{
			DrawMeshGroup(cb, group, *m_deviceCommandsBuffer, offset, VK_INDEX_TYPE_UINT32);
		}
	}
	
	void ChunkRenderList::RenderWater(CommandBuffer& cb) const
	{
		uint64_t offset = m_waterIndirectCommandsOffset * sizeof(VkDrawIndexedIndirectCommand);
		
		for (const WaterMeshGroup& group : m_waterMeshGroups)
		{
			DrawMeshGroup(cb, group, *m_deviceCommandsBuffer, offset, VK_INDEX_TYPE_UINT16);
		}
	}
}
