#pragma once

#include "../vulkan/vk.h"
#include "shaders/skyshader.h"
#include "shaders/godraysgenshader.h"
#include "shaders/godraysblurshader.h"

namespace MCR
{
	class PostProcessor
	{
	public:
		struct Timestamps
		{
			std::chrono::duration<float, std::milli> m_godRays;
			std::chrono::duration<float, std::milli> m_sky;
		};
		
		PostProcessor(const VkDescriptorBufferInfo& renderSettingsBufferInfo);
		
		void SetRenderSettings(const VkDescriptorBufferInfo& renderSettingsBufferInfo);
		
		void FramebufferChanged(const class Framebuffer& framebuffer);
		
		Timestamps GetElapsedTime() const;
		
		inline VkCommandBuffer GetCommandBuffer() const
		{
			return m_commandBuffers[frameQueueIndex].GetVkCB();
		}
		
		inline VkRenderPass GetGodRaysRenderPass() const
		{
			return *m_godRaysRenderPass;
		}
		
		inline VkRenderPass GetSkyRenderPass() const
		{
			return *m_skyRenderPass;
		}
		
		static constexpr VkFormat GodRaysFormat = VK_FORMAT_R16_SFLOAT;
		static constexpr uint32_t GodRaysDownscale = 2;
		
	private:
		VkHandle<VkRenderPass> m_godRaysRenderPass; //Used for both generation and blurring.
		VkHandle<VkRenderPass> m_skyRenderPass;
		
		VkHandle<VkQueryPool> m_timestampQueryPool;
		
		GodRaysGenShader m_godRaysGenShader;
		GodRaysBlurShader m_godRaysBlurShader;
		SkyShader m_skyShader;
		
		std::vector<CommandBuffer> m_commandBuffers;
	};
}
