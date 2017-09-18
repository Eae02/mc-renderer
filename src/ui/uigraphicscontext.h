#pragma once

#include "uishader.h"

namespace MCR
{
	class UIGraphicsContext
	{
	public:
		UIGraphicsContext();
		
		void Draw(const class UIDrawList& drawList, const class Framebuffer& framebuffer);
		
		inline VkCommandBuffer GetCommandBuffer() const
		{
			return m_commandBuffers[frameQueueIndex].GetVkCB();
		}
		
		inline VkRenderPass GetRenderPass() const
		{
			return *m_renderPass;
		}
		
	private:
		VkHandle<VkRenderPass> m_renderPass;
		
		VkHandle<VkSampler> m_sampler;
		UniqueDescriptorSet m_samplerDescriptorSet;
		
		UIShader m_shader;
		
		std::array<CommandBuffer, MaxQueuedFrames> m_commandBuffers;
		
		uint64_t m_bufferSize = 0;
		
		VkHandle<VmaAllocation> m_hostBufferAllocation;
		VkHandle<VkBuffer> m_hostBuffer;
		std::array<void*, 3> m_hostBufferMemory;
		
		VkHandle<VmaAllocation> m_deviceBufferAllocation;
		VkHandle<VkBuffer> m_deviceBuffer;
	};
}
