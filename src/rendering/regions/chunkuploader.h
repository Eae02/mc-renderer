#pragma once

#include <cstdint>

#include "chunkmesh.h"
#include "../../vulkan/vk.h"

namespace MCR
{
	class ChunkUploader
	{
	public:
		ChunkUploader();
		
		void BeginUploading(int64_t x, int64_t y, int64_t z, Region::ChunkConnectivity connectivity,
		                    const class MeshBuilder& meshBuilder);
		
		void WaitIdle();
		
		//Signature for CallbackTp: (int64_t x, int64_t y, int64_t z, ChunkMesh& mesh)
		template <typename CallbackTp>
		void IterateCompleted(CallbackTp callback)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			
			for (long i = m_tasks.size() - 1; i >= 0; i--)
			{
				if (vkGetFenceStatus(vulkan.device, *m_tasks[i].m_hostBuffer.m_fence) == VK_SUCCESS)
				{
					callback(m_tasks[i].m_x, m_tasks[i].m_y, m_tasks[i].m_z, m_tasks[i].m_chunk);
					
					m_hostBuffers.push_back(std::move(m_tasks[i].m_hostBuffer));
					
					if (i != static_cast<long>(m_tasks.size()) - 1)
					{
						m_tasks[i] = std::move(m_tasks.back());
					}
					m_tasks.pop_back();
				}
			}
		}
		
	private:
		struct HostBuffer
		{
			explicit HostBuffer(uint64_t size);
			
			uint64_t m_size;
			VkHandle<VkFence> m_fence;
			VkHandle<VkBuffer> m_buffer;
			VkHandle<VmaAllocation> m_allocation;
			void* m_memory;
		};
		
		std::mutex m_mutex;
		
		HostBuffer AllocateHostBuffer(uint64_t size);
		
		std::vector<HostBuffer> m_hostBuffers;
		
		VkHandle<VkCommandPool> m_commandPool;
		
		struct Task
		{
			int64_t m_x;
			int64_t m_y;
			int64_t m_z;
			ChunkMesh m_chunk;
			HostBuffer m_hostBuffer;
			CommandBuffer m_cb;
		};
		
		std::vector<Task> m_tasks;
	};
}
