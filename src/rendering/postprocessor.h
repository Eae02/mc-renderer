#pragma once

#include "../vulkan/vk.h"
#include "shaders/postprocessshader.h"

namespace MCR
{
	class PostProcessor
	{
	public:
		PostProcessor();
		
		void FramebufferChanged(const class Framebuffer& framebuffer);
		
		inline VkCommandBuffer GetCommandBuffer() const
		{
			return m_commandBuffers[frameQueueIndex].GetVkCB();
		}
		
	private:
		VkHandle<VkRenderPass> m_finalRenderPass;
		
		PostProcessShader m_postProcessShader;
		
		std::vector<CommandBuffer> m_commandBuffers;
	};
}
