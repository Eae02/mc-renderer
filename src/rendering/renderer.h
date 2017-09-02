#pragma once

#include "../vulkan/vk.h"
#include "rendererframebuffer.h"

namespace MCR
{
	class Renderer
	{
	public:
		Renderer();
		
		struct RenderParams
		{
			const RendererFramebuffer* m_framebuffer;
			VkSemaphore m_signalSemaphore;
			VkFence m_signalFence;
		};
		
		void Render(const RenderParams& params);
		
		inline VkRenderPass GetRenderPass() const
		{
			return *m_renderPass;
		}
		
		static constexpr VkFormat ColorAttachmentFormat = VK_FORMAT_R8G8B8A8_UNORM;
		
	private:
		VkHandle<VkRenderPass> m_renderPass;
		
		std::array<CommandBuffer, MaxQueuedFrames> m_commandBuffers;
	};
}
