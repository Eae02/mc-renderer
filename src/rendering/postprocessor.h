#pragma once

#include "../vulkan/vk.h"

namespace MCR
{
	class PostProcessor
	{
	public:
		PostProcessor();
		
		void SetRenderSettings(const VkDescriptorBufferInfo& renderSettingsBufferInfo);
		
		void FramebufferChanged(const class Framebuffer& framebuffer);
		
		inline VkCommandBuffer GetCommandBuffer() const
		{
			return m_commandBuffer.GetVkCB();
		}
		
	private:
		VkHandle<VkPipelineLayout> m_pipelineLayout;
		VkHandle<VkPipeline> m_pipeline;
		
		VkHandle<VkSampler> m_depthSampler;
		
		CommandBuffer m_commandBuffer;
		
		UniqueDescriptorSet m_descriptorSet;
	};
}
