#pragma once

#include "../vulkan/vk.h"
#include "frustum.h"
#include "regionrenderlist.h"
#include "framebuffer.h"
#include "rendersettingsbuffer.h"
#include "shaders/blockshader.h"
#include "postprocessor.h"

namespace MCR
{
	class WorldManager;
	
	class Renderer
	{
	public:
		Renderer();
		
		struct RenderParams
		{
			float m_time;
			class FrameProfiler* m_frameProfiler;
			const class TimeManager* m_timeManager;
		};
		
		void Render(const RenderParams& params);
		
		inline VkCommandBuffer GetCommandBuffer() const
		{
			return m_commandBuffers[frameQueueIndex].GetVkCB();
		}
		
		inline void SetFrustumFrozen(bool frozen)
		{
			m_isFrustumFrozen = frozen;
		}
		
		inline bool IsFrustumFrozen() const
		{
			return m_isFrustumFrozen;
		}
		
		inline void SetWireframe(bool wireframe)
		{
			m_wireframe = wireframe;
		}
		
		inline bool Wireframe() const
		{
			return m_wireframe;
		}
		
		inline VkRenderPass GetRenderPass() const
		{
			return *m_renderPass;
		}
		
		inline const VkDescriptorBufferInfo& GetRenderSettingsBufferInfo() const
		{
			return m_renderSettingsBuffer.GetBufferInfo();
		}
		
		inline void SetWorldManager(const WorldManager* worldManager)
		{
			m_worldManager = worldManager;
		}
		
		void FramebufferChanged(const Framebuffer& framebuffer);
		
		static constexpr VkFormat ColorAttachmentFormat = VK_FORMAT_R8G8B8A8_UNORM;
		
	private:
		static VkRenderPass CreateRenderPass();
		
		VkHandle<VkRenderPass> m_renderPass;
		
		RenderSettingsBuffer m_renderSettingsBuffer;
		
		PostProcessor m_postProcessor;
		
		BlockShader m_blockShader;
		
		RegionRenderList m_regionRenderList;
		
		const Framebuffer* m_framebuffer;
		
		std::array<CommandBuffer, MaxQueuedFrames> m_commandBuffers;
		
		bool m_isFrustumFrozen = false;
		Frustum m_frustum;
		
		bool m_wireframe = false;
		
		glm::mat4 m_projectionMatrix;
		glm::mat4 m_invProjectionMatrix;
		
		const WorldManager* m_worldManager = nullptr;
	};
}
