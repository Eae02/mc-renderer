#pragma once

#include "../vulkan/vk.h"
#include "shaders/skyshader.h"
#include "shaders/godraysgenshader.h"
#include "shaders/godraysblurshader.h"

namespace MCR
{
	class SkyRenderer
	{
	public:
		explicit SkyRenderer(const VkDescriptorBufferInfo& renderSettingsBufferInfo);
		
		void FramebufferChanged(const class Framebuffer& framebuffer);
		
		void Render(CommandBuffer& commandBuffer, const class Framebuffer& framebuffer);
		
		inline VkRenderPass GetGodRaysRenderPass() const
		{
			return *m_godRaysRenderPass;
		}
		
		inline VkRenderPass GetSkyRenderPass() const
		{
			return *m_skyRenderPass;
		}
		
		static constexpr uint32_t GodRaysDownscale = 2;
		
	private:
		VkHandle<VkRenderPass> m_godRaysRenderPass; //Used for both generation and blurring.
		VkHandle<VkRenderPass> m_skyRenderPass;
		
		GodRaysGenShader m_godRaysGenShader;
		GodRaysBlurShader m_godRaysBlurShader;
		SkyShader m_skyShader;
	};
}
