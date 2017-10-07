#pragma once

#include "../vulkan/vk.h"
#include "shaders/skyshader.h"

namespace MCR
{
	class PostProcessor
	{
	public:
		PostProcessor(const VkDescriptorBufferInfo& renderSettingsBufferInfo);
		
		void SetRenderSettings(const VkDescriptorBufferInfo& renderSettingsBufferInfo);
		
		void FramebufferChanged(const class Framebuffer& framebuffer);
		
		inline VkCommandBuffer GetCommandBuffer() const
		{
			return m_commandBuffers[frameQueueIndex].GetVkCB();
		}
		
	private:
		VkHandle<VkRenderPass> m_renderPass;
		
		SkyShader m_skyShader;
		
		std::vector<CommandBuffer> m_commandBuffers;
	};
}
