#include "chunkvisibilitygraph.h"
#include "../blocks/sides.h"

namespace MCR
{
	ChunkVisibilityGraph::ChunkVisibilityGraph(const bool* visitedChunks, int regionTableSize,
	                                           RegionCoordinate centerRegion, CommandBuffer& commandBuffer)
	{
		auto Visited = [&] (const glm::ivec3& pos)
		{
			return visitedChunks[(pos.z + pos.y * regionTableSize) * regionTableSize + pos.x];
		};
		
		std::vector<glm::vec3> vertices;
		std::vector<uint32_t> indices;
		
		const int64_t toWorldX = centerRegion.x - regionTableSize / 2;
		const int64_t toWorldZ = centerRegion.z - regionTableSize / 2;
		
		const uint32_t faceIndices[] = { 0, 1, 2, 2, 1, 3 };
		
		for (int y = 0; y < Region::ChunkCount; y++)
		{
			for (int z = 0; z < regionTableSize; z++)
			{
				for (int x = 0; x < regionTableSize; x++)
				{
					const glm::ivec3 pos(x, y, z);
					
					if (!Visited(pos))
						continue;
					
					const glm::vec3 worldCenter = (glm::vec3(x + toWorldX, y, z + toWorldZ) + 0.5f) *
					                              static_cast<float>(Region::Size);
					
					for (int side = 0; side < 6; side++)
					{
						glm::ivec3 sidePos = glm::ivec3(x, y, z) + BlockNormals[side];
						
						if (sidePos.x < 0 || sidePos.y < 0 || sidePos.z < 0 || sidePos.x >= regionTableSize || 
						    sidePos.y >= Region::ChunkCount || sidePos.z >= regionTableSize || Visited(sidePos))
						{
							continue;
						}
						
						const glm::vec3 faceCenter = worldCenter + glm::vec3(BlockNormals[side] * (Region::Size / 2));
						
						const uint32_t baseIndex = vertices.size();
						for (uint32_t i : faceIndices)
						{
							indices.push_back(baseIndex + i);
						}
						
						const glm::vec3 up = BlockTangents[side] * Region::Size;
						const glm::vec3 left = BlockBiTangents[side] * Region::Size;
						
						for (int vx = 0; vx < 2; vx++)
						{
							for (int vy = 0; vy < 2; vy++)
							{
								vertices.push_back(faceCenter + up * (vy - 0.5f) + left * (vx - 0.5f));
							}
						}
					}
				}
			}
		}
		
		m_numIndices = indices.size();
		
		const uint64_t verticesBytes = vertices.size() * sizeof(glm::vec3);
		const uint64_t indicesBytes = indices.size() * sizeof(uint32_t);
		
		// ** Allocates a shared vertex & index buffer **
		VkBufferCreateInfo bufferCreateInfo;
		InitBufferCreateInfo(bufferCreateInfo, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
		                     VK_BUFFER_USAGE_INDEX_BUFFER_BIT, verticesBytes + indicesBytes);
		
		const VmaMemoryRequirements memoryRequirements = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
		
		CheckResult(vmaCreateBuffer(vulkan.allocator, &bufferCreateInfo, &memoryRequirements,
		                            m_buffer.GetCreateAddress(), m_allocation.GetCreateAddress(), nullptr));
		
		// ** Uploads data to the buffer **
		commandBuffer.UpdateBuffer(*m_buffer, 0,            indicesBytes,  indices.data());
		commandBuffer.UpdateBuffer(*m_buffer, indicesBytes, verticesBytes, vertices.data());
		
		// ** Inserts barriers for the upload **
		VkBufferMemoryBarrier barriers[2];
		
		//Barrier for indices
		barriers[0] = 
		{
			/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			/* pNext               */ nullptr,
			/* srcAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
			/* dstAccessMask       */ VK_ACCESS_INDEX_READ_BIT,
			/* srcQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
			/* dstQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
			/* buffer              */ *m_buffer,
			/* offset              */ 0,
			/* size                */ indicesBytes
		};
		
		//Barriers for vertices
		barriers[1] = 
		{
			/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			/* pNext               */ nullptr,
			/* srcAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
			/* dstAccessMask       */ VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
			/* srcQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
			/* dstQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
			/* buffer              */ *m_buffer,
			/* offset              */ indicesBytes,
			/* size                */ verticesBytes
		};
		
		commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
		                              0, { }, barriers, { });
	}
	
	void ChunkVisibilityGraph::Draw(CommandBuffer& commandBuffer) const
	{
		commandBuffer.BindIndexBuffer(*m_buffer, 0, VK_INDEX_TYPE_UINT32);
		
		VkDeviceSize vertexOffsets[] = { m_numIndices * sizeof(uint32_t) };
		commandBuffer.BindVertexBuffers(0, 1, &*m_buffer, vertexOffsets);
		
		commandBuffer.DrawIndexed(m_numIndices, 1, 0, 0, 0);
	}
}
