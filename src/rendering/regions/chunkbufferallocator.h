#pragma once

#include "../../vulkan/vk.h"
#include "../../poolallocationtracker.h"

#include <vector>
#include <memory>
#include <optional>

namespace MCR
{
	class ChunkBufferAllocator
	{
	public:
		class Allocation
		{
			friend class ChunkBufferAllocator;
			
		public:
			inline Allocation()
			{
				m_data.m_vertexBuffer = VK_NULL_HANDLE;
			}
			
			inline ~Allocation()
			{
				Reset();
			}
			
			inline Allocation(Allocation&& other)
			    : m_data(other.m_data)
			{
				other.m_data.m_vertexBuffer = VK_NULL_HANDLE;
			}
			
			inline Allocation& operator=(Allocation&& other)
			{
				Reset();
				
				m_data = other.m_data;
				
				other.m_data.m_vertexBuffer = VK_NULL_HANDLE;
				
				return *this;
			}
			
			inline void Reset()
			{
				if (m_data.m_vertexBuffer)
				{
					s_instance.Free(*this);
					m_data.m_vertexBuffer = VK_NULL_HANDLE;
				}
			}
			
			inline bool HasData() const
			{
				return m_data.m_vertexBuffer != VK_NULL_HANDLE;
			}
			
			void BeforeTransfer(CommandBuffer& commandBuffer);
			void AfterTransfer(CommandBuffer& commandBuffer);
			
			void PrepareForRendering(CommandBuffer& commandBuffer);
			
			inline VkBuffer GetVertexBuffer() const
			{
				return m_data.m_vertexBuffer;
			}
			
			inline VkBuffer GetIndexBuffer() const
			{
				return m_data.m_indexBuffer;
			}
			
			inline uint64_t GetVertexOffset() const
			{
				return m_data.m_vertexOffset;
			}
			
			inline uint64_t GetIndexOffset() const
			{
				return m_data.m_indexOffset;
			}
			
			inline uint64_t GetNumVertices() const
			{
				return m_data.m_numVertices;
			}
			
			inline uint64_t GetNumIndices() const
			{
				return m_data.m_numIndices;
			}
			
			inline void MarkUsed()
			{
				m_data.m_lastUsedFrameIndex = frameIndex;
			}
			
		private:
			struct Data
			{
				VkBuffer m_vertexBuffer;
				VkBuffer m_indexBuffer;
				
				uint64_t m_vertexOffset;
				uint64_t m_indexOffset;
				
				uint64_t m_numVertices;
				uint64_t m_numIndices;
				
				uint64_t m_lastUsedFrameIndex;
				
				bool m_aquiredByGraphicsQueue;
			} m_data;
			
			inline Allocation(const Data& data)
			    : m_data(data) { }
		};
		
		Allocation Allocate(uint64_t numIndices, uint64_t numVertices);
		
		void Free(const Allocation& allocation);
		
		void ProcessFreedAllocations(CommandBuffer& commandBuffer);
		
		inline void ReleaseMemory()
		{
			m_pages.clear();
		}
		
		static ChunkBufferAllocator s_instance;
		
	private:
		void FreeAllocationData(const Allocation::Data& allocation);
		
		ChunkBufferAllocator() = default;
		
		//Chunks usually use ~4000 indices and 3000 vertices.
		static constexpr uint64_t IndicesPerPage = 4096 * 2048;
		static constexpr uint64_t VerticesPerPage = 3072 * 2048;
		
		std::mutex m_mutex;
		
		struct DataPage
		{
			VkHandle<VmaAllocation> m_indexAllocation;
			VkHandle<VmaAllocation> m_vertexAllocation;
			
			VkHandle<VkBuffer> m_indexBuffer;
			VkHandle<VkBuffer> m_vertexBuffer;
			
			PoolAllocationTracker m_indexAllocationTracker;
			PoolAllocationTracker m_vertexAllocationTracker;
			
			DataPage();
		};
		
		std::vector<DataPage> m_pages;
		
		//Freed allocations that are still in use.
		std::vector<Allocation::Data> m_freedAllocationsInUse;
	};
}
