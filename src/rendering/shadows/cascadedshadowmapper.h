#pragma once

#include "../../vulkan/vk.h"
#include "../shaders/shader.h"

namespace MCR
{
	class CascadedShadowMapper
	{
	public:
		CascadedShadowMapper();
		
		void SetResolution(uint32_t resolution);
		
		void Render();
		
		static constexpr uint32_t NumCascades = 3;
		
	private:
		VkHandle<VkRenderPass> m_renderPass;
		
		Shader m_shader;
		
		uint32_t m_resolution;
		
		VkHandle<VkImage, VkHandleDestroyTime::Delayed> m_shadowMap;
		VkHandle<VkImageView, VkHandleDestroyTime::Delayed> m_shadowMapView;
		VkHandle<VkFramebuffer, VkHandleDestroyTime::Delayed> m_framebuffer;
		
		
	};
}
