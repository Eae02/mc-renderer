#pragma once

#include "../vulkan/vk.h"
#include "frustum.h"
#include "regionrenderlist.h"
#include "rendererframebuffer.h"
#include "rendersettingsbuffer.h"
#include "shaders/blockshader.h"

namespace MCR
{
	class WorldManager;
	
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
		
		inline void SetFrustumFrozen(bool frozen)
		{
			m_isFrustumFrozen = frozen;
		}
		
		inline bool IsFrustumFrozen() const
		{
			return m_isFrustumFrozen;
		}
		
		inline VkRenderPass GetRenderPass() const
		{
			return *m_renderPass;
		}
		
		inline void SetWorldManager(const WorldManager* worldManager)
		{
			m_worldManager = worldManager;
		}
		
		static constexpr VkFormat ColorAttachmentFormat = VK_FORMAT_R8G8B8A8_UNORM;
		
	private:
		static VkRenderPass CreateRenderPass();
		
		VkHandle<VkRenderPass> m_renderPass;
		
		RenderSettingsBuffer m_renderSettingsBuffer;
		
		BlockShader m_blockShader;
		
		RegionRenderList m_regionRenderList;
		
		std::array<CommandBuffer, MaxQueuedFrames> m_commandBuffers;
		
		bool m_isFrustumFrozen = false;
		Frustum m_frustum;
		
		uint32_t m_projWidth = 0;
		uint32_t m_projHeight = 0;
		glm::mat4 m_projectionMatrix;
		glm::mat4 m_invProjectionMatrix;
		
		const WorldManager* m_worldManager = nullptr;
	};
}
