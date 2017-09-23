#include "regionrenderlist.h"

namespace MCR
{
	void RegionRenderList::Begin()
	{
		for (MeshGroup& group : m_meshGroups)
		{
			group.m_meshes.clear();
		}
		
		m_numMeshes = 0;
	}
	
	void RegionRenderList::Add(ChunkMesh& mesh)
	{
		auto groupIt = std::find_if(MAKE_RANGE(m_meshGroups), [&] (const MeshGroup& group)
		{
			return group.m_vertexBuffer == mesh.GetVertexBuffer() && group.m_indexBuffer == mesh.GetIndexBuffer();
		});
		
		if (groupIt != m_meshGroups.end())
		{
			groupIt->m_meshes.push_back(&mesh);
		}
		else
		{
			m_meshGroups.emplace_back(mesh);
		}
		
		m_numMeshes++;
	}
	
	void RegionRenderList::End(CommandBuffer& cb)
	{
		if (m_numMeshes == 0)
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
		
		if (m_numAllocatedCommands < m_numMeshes)
		{
			m_numAllocatedCommands = RoundToNextMultiple<uint32_t>(m_numMeshes, 1024);
			
			const uint64_t bufferSize = m_numAllocatedCommands * sizeof(VkDrawIndexedIndirectCommand);
			
			// ** Allocates the host buffer **
			const VmaMemoryRequirements hostBufferMemRequirements =
			{
				VMA_MEMORY_REQUIREMENT_PERSISTENT_MAP_BIT,
				VMA_MEMORY_USAGE_CPU_ONLY
			};
			
			VkBufferCreateInfo hostBufferCreateInfo;
			InitBufferCreateInfo(hostBufferCreateInfo, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, bufferSize * MaxQueuedFrames);
			
			VmaAllocationInfo hostAllocationInfo;
			
			CheckResult(vmaCreateBuffer(vulkan.allocator, &hostBufferCreateInfo, &hostBufferMemRequirements,
			                            m_hostCommandsBuffer.GetCreateAddress(),
			                            m_hostCommandsAllocation.GetCreateAddress(), &hostAllocationInfo));
			
			m_hostCommandsMemory = reinterpret_cast<VkDrawIndexedIndirectCommand*>(hostAllocationInfo.pMappedData);
			
			// ** Allocates the device buffer **
			const VmaMemoryRequirements deviceBufferMemRequirements = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
			
			VkBufferCreateInfo deviceBufferCreateInfo;
			InitBufferCreateInfo(deviceBufferCreateInfo, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
			                     VK_BUFFER_USAGE_TRANSFER_DST_BIT, bufferSize);
			
			CheckResult(vmaCreateBuffer(vulkan.allocator, &deviceBufferCreateInfo, &deviceBufferMemRequirements,
			                            m_deviceCommandsBuffer.GetCreateAddress(),
			                            m_deviceCommandsAllocation.GetCreateAddress(), nullptr));
		}
		
		uint32_t hostCommandsOffset = m_numAllocatedCommands * frameQueueIndex;
		VkDrawIndexedIndirectCommand* nextHostCommand = m_hostCommandsMemory + hostCommandsOffset;
		
		for (MeshGroup& group : m_meshGroups)
		{
			for (ChunkMesh* mesh : group.m_meshes)
			{
				mesh->PrepareForRendering(cb);
				mesh->WriteIndirectCommand(*(nextHostCommand++));
			}
		}
		
		// ** Uploads indirect commands. **
		const VkBufferCopy commandsBufferCopy =
		{
			/* srcOffset */ hostCommandsOffset * sizeof(VkDrawIndexedIndirectCommand),
			/* dstOffset */ 0,
			/* size      */ m_numMeshes * sizeof(VkDrawIndexedIndirectCommand)
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
	
	void RegionRenderList::Render(CommandBuffer& cb) const
	{
		uint64_t offset = 0;
		
		for (const MeshGroup& group : m_meshGroups)
		{
			cb.BindIndexBuffer(group.m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
			
			VkDeviceSize vbOffsets[] = { 0 };
			cb.BindVertexBuffers(0, 1, &group.m_vertexBuffer, vbOffsets);
			
			if (vulkan.limits.hasMultiDrawIndirect)
			{
				cb.DrawIndexedIndirect(*m_deviceCommandsBuffer, offset, group.m_meshes.size(), sizeof(VkDrawIndexedIndirectCommand));
				
				offset += group.m_meshes.size() * sizeof(VkDrawIndexedIndirectCommand);
			}
			else
			{
				VkDrawIndexedIndirectCommand command;
				
				for (const ChunkMesh* mesh : group.m_meshes)
				{
					mesh->WriteIndirectCommand(command);
					
					cb.DrawIndexed(command.indexCount, command.instanceCount, command.firstIndex,
					               command.vertexOffset, command.firstInstance);
				}
			}
		}
	}
}
