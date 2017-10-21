#include "chunkbufferallocator.h"
#include "../vertex.h"

#include <algorithm>

namespace MCR
{
	ChunkBufferAllocator ChunkBufferAllocator::s_instance;
	
	ChunkBufferAllocator::DataPage::DataPage()
	    : m_indexAllocationTracker(IndicesPerPage), m_vertexAllocationTracker(VerticesPerPage)
	{
		const VmaAllocationCreateInfo allocationCI = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
		
		VkBufferCreateInfo vertexBufferCreateInfo;
		InitBufferCreateInfo(vertexBufferCreateInfo, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
		                     VK_BUFFER_USAGE_TRANSFER_DST_BIT, VerticesPerPage * sizeof(Vertex));
		
		CheckResult(vmaCreateBuffer(vulkan.allocator, &vertexBufferCreateInfo, &allocationCI,
		                            m_vertexBuffer.GetCreateAddress(), m_vertexAllocation.GetCreateAddress(), nullptr));
		
		VkBufferCreateInfo indexBufferCreateInfo;
		InitBufferCreateInfo(indexBufferCreateInfo, VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
		                     VK_BUFFER_USAGE_TRANSFER_DST_BIT, IndicesPerPage * sizeof(uint32_t));
		
		CheckResult(vmaCreateBuffer(vulkan.allocator, &indexBufferCreateInfo, &allocationCI,
		                            m_indexBuffer.GetCreateAddress(), m_indexAllocation.GetCreateAddress(), nullptr));
	}
	
	ChunkBufferAllocator::Allocation ChunkBufferAllocator::Allocate(uint64_t numIndices, uint64_t numVertices)
	{
		if (numIndices > IndicesPerPage || numVertices > VerticesPerPage)
		{
			Log("Too many vertices / indices. I:", numIndices, ", V:", numVertices);
			std::exit(1);
		}
		
		std::lock_guard<std::mutex> lock(m_mutex);
		
	begin:
		for (auto page = m_pages.rbegin(); page != m_pages.rend(); ++page)
		{
			auto availIndexAllocation = page->m_indexAllocationTracker.FindAvailable(numIndices);
			auto availVertexAllocation = page->m_vertexAllocationTracker.FindAvailable(numVertices);
			
			if (availIndexAllocation.Found() && availVertexAllocation.Found())
			{
				page->m_indexAllocationTracker.Allocate(availIndexAllocation, numIndices);
				page->m_vertexAllocationTracker.Allocate(availVertexAllocation, numVertices);
				
				Allocation::Data allocationData;
				allocationData.m_vertexBuffer = *page->m_vertexBuffer;
				allocationData.m_indexBuffer = *page->m_indexBuffer;
				allocationData.m_vertexOffset = availVertexAllocation.GetFirstElement();
				allocationData.m_indexOffset = availIndexAllocation.GetFirstElement();
				allocationData.m_numVertices = numVertices;
				allocationData.m_numIndices = numIndices;
				allocationData.m_lastUsedFrameIndex = 0;
				allocationData.m_aquiredByGraphicsQueue = false;
				
				return Allocation(allocationData);
			}
		}
		
		m_pages.emplace_back();
		Log("Creating new chunk buffer page");
		goto begin;
	}
	
	void ChunkBufferAllocator::Free(const ChunkBufferAllocator::Allocation& allocation)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		
		if (allocation.m_data.m_lastUsedFrameIndex + SwapChain::GetImageCount() > frameIndex)
		{
			m_freedAllocationsInUse.push_back(allocation.m_data);
		}
		else
		{
			FreeAllocationData(allocation.m_data);
		}
	}
	
	inline bool TransferQueueIsGraphicsQueue()
	{
		return vulkan.queueFamilies[QUEUE_FAMILY_GRAPHICS] == vulkan.queueFamilies[QUEUE_FAMILY_TRANSFER];
	}
	
	void ChunkBufferAllocator::ProcessFreedAllocations(CommandBuffer& commandBuffer)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		
		for (long i = static_cast<long>(m_freedAllocationsInUse.size()) - 1; i >= 0; i--)
		{
			if (m_freedAllocationsInUse[i].m_lastUsedFrameIndex + SwapChain::GetImageCount() <= frameIndex)
			{
				if (m_freedAllocationsInUse[i].m_aquiredByGraphicsQueue && !TransferQueueIsGraphicsQueue())
				{
					VkBufferMemoryBarrier barriers[2];
					
					//Releases the vertex buffer from the graphics queue.
					barriers[0] = 
					{
						/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
						/* pNext               */ nullptr,
						/* srcAccessMask       */ VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
						/* dstAccessMask       */ 0,
						/* srcQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_GRAPHICS],
						/* dstQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_TRANSFER],
						/* buffer              */ m_freedAllocationsInUse[i].m_vertexBuffer,
						/* offset              */ m_freedAllocationsInUse[i].m_vertexOffset * sizeof(Vertex),
						/* size                */ m_freedAllocationsInUse[i].m_numVertices * sizeof(Vertex)
					};
					
					//Releases the index buffer from the graphics queue.
					barriers[1] = 
					{
						/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
						/* pNext               */ nullptr,
						/* srcAccessMask       */ VK_ACCESS_INDEX_READ_BIT,
						/* dstAccessMask       */ 0,
						/* srcQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_GRAPHICS],
						/* dstQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_TRANSFER],
						/* buffer              */ m_freedAllocationsInUse[i].m_indexBuffer,
						/* offset              */ m_freedAllocationsInUse[i].m_indexOffset * sizeof(uint32_t),
						/* size                */ m_freedAllocationsInUse[i].m_numIndices * sizeof(uint32_t)
					};
					
					commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
					                              0, { }, barriers, { });
				}
				
				//This allocation is no longer in use, so it can be freed.
				FreeAllocationData(m_freedAllocationsInUse[i]);
				
				//Removes the allocation from the list.
				if (i != static_cast<long>(m_freedAllocationsInUse.size()) - 1)
				{
					m_freedAllocationsInUse[i] = m_freedAllocationsInUse.back();
				}
				m_freedAllocationsInUse.pop_back();
			}
		}
	}
	
	void ChunkBufferAllocator::ReleaseMemory()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_pages.clear();
		m_freedAllocationsInUse.clear();
	}
	
	void ChunkBufferAllocator::FreeAllocationData(const ChunkBufferAllocator::Allocation::Data& allocation)
	{
		for (DataPage& page : m_pages)
		{
			if (*page.m_vertexBuffer == allocation.m_vertexBuffer)
			{
				page.m_indexAllocationTracker.Free(allocation.m_indexOffset, allocation.m_numIndices);
				page.m_vertexAllocationTracker.Free(allocation.m_vertexOffset, allocation.m_numVertices);
				
				break;
			}
		}
	}
	
	void ChunkBufferAllocator::Allocation::BeforeTransfer(CommandBuffer& commandBuffer)
	{
		if (!TransferQueueIsGraphicsQueue())
		{
			VkBufferMemoryBarrier barriers[2];
			
			//Aquires the vertex buffer from the graphics queue.
			barriers[0] = 
			{
				/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				/* pNext               */ nullptr,
				/* srcAccessMask       */ 0,
				/* dstAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
				/* srcQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_GRAPHICS],
				/* dstQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_TRANSFER],
				/* buffer              */ m_data.m_vertexBuffer,
				/* offset              */ m_data.m_vertexOffset * sizeof(Vertex),
				/* size                */ m_data.m_numVertices * sizeof(Vertex)
			};
			
			//Aquires the index buffer from the graphics queue.
			barriers[1] = 
			{
				/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				/* pNext               */ nullptr,
				/* srcAccessMask       */ 0,
				/* dstAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
				/* srcQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_GRAPHICS],
				/* dstQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_TRANSFER],
				/* buffer              */ m_data.m_indexBuffer,
				/* offset              */ m_data.m_indexOffset * sizeof(uint32_t),
				/* size                */ m_data.m_numIndices * sizeof(uint32_t)
			};
			
			commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			                              { }, barriers, { });
		}
	}
	
	void ChunkBufferAllocator::Allocation::AfterTransfer(CommandBuffer& commandBuffer)
	{
		if (!TransferQueueIsGraphicsQueue())
		{
			VkBufferMemoryBarrier barriers[2];
			
			//Releases the vertex buffer from the transfer queue.
			barriers[0] = 
			{
				/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				/* pNext               */ nullptr,
				/* srcAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
				/* dstAccessMask       */ 0,
				/* srcQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_TRANSFER],
				/* dstQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_GRAPHICS],
				/* buffer              */ m_data.m_vertexBuffer,
				/* offset              */ m_data.m_vertexOffset * sizeof(Vertex),
				/* size                */ m_data.m_numVertices * sizeof(Vertex)
			};
			
			//Releases the index buffer from the transfer queue.
			barriers[1] = 
			{
				/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				/* pNext               */ nullptr,
				/* srcAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
				/* dstAccessMask       */ 0,
				/* srcQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_TRANSFER],
				/* dstQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_GRAPHICS],
				/* buffer              */ m_data.m_indexBuffer,
				/* offset              */ m_data.m_indexOffset * sizeof(uint32_t),
				/* size                */ m_data.m_numIndices * sizeof(uint32_t)
			};
			
			commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
			                              { }, barriers, { });
		}
		else
		{
			VkBufferMemoryBarrier barriers[2];
			
			//Inserts a barrier for the vertex buffer.
			barriers[0] = 
			{
				/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				/* pNext               */ nullptr,
				/* srcAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
				/* dstAccessMask       */ VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
				/* srcQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
				/* dstQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
				/* buffer              */ m_data.m_vertexBuffer,
				/* offset              */ m_data.m_vertexOffset * sizeof(Vertex),
				/* size                */ m_data.m_numVertices * sizeof(Vertex)
			};
			
			//Inserts a barrier for the index buffer.
			barriers[1] = 
			{
				/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				/* pNext               */ nullptr,
				/* srcAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
				/* dstAccessMask       */ VK_ACCESS_INDEX_READ_BIT,
				/* srcQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
				/* dstQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
				/* buffer              */ m_data.m_indexBuffer,
				/* offset              */ m_data.m_indexOffset * sizeof(uint32_t),
				/* size                */ m_data.m_numIndices * sizeof(uint32_t)
			};
			
			commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
			                              { }, barriers, { });
		}
	}
	
	void ChunkBufferAllocator::Allocation::PrepareForRendering(CommandBuffer& commandBuffer)
	{
		if (!m_data.m_aquiredByGraphicsQueue && !TransferQueueIsGraphicsQueue())
		{
			VkBufferMemoryBarrier barriers[2];
			
			//Aquires the vertex buffer from the transfer queue.
			barriers[0] = 
			{
				/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				/* pNext               */ nullptr,
				/* srcAccessMask       */ 0,
				/* dstAccessMask       */ VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
				/* srcQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_TRANSFER],
				/* dstQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_GRAPHICS],
				/* buffer              */ m_data.m_vertexBuffer,
				/* offset              */ m_data.m_vertexOffset * sizeof(Vertex),
				/* size                */ m_data.m_numVertices * sizeof(Vertex)
			};
			
			//Aquires the index buffer from the transfer queue.
			barriers[1] = 
			{
				/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				/* pNext               */ nullptr,
				/* srcAccessMask       */ 0,
				/* dstAccessMask       */ VK_ACCESS_INDEX_READ_BIT,
				/* srcQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_TRANSFER],
				/* dstQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_GRAPHICS],
				/* buffer              */ m_data.m_indexBuffer,
				/* offset              */ m_data.m_indexOffset * sizeof(uint32_t),
				/* size                */ m_data.m_numIndices * sizeof(uint32_t)
			};
			
			commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
			                              { }, barriers, { });
			
			m_data.m_aquiredByGraphicsQueue = true;
		}
	}
}
