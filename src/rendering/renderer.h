#pragma once

#include "../vulkan/vk.h"
#include "frustum.h"
#include "chunkrenderlist.h"
#include "framebuffer.h"
#include "rendersettingsbuffer.h"
#include "shaders/blockshader.h"
#include "shaders/debugshader.h"
#include "postprocessor.h"
#include "chunkvisibilitycalculator.h"
#include "shadows/cascadedshadowmapper.h"

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
		
		inline void SetEnableOcclusionCulling(bool enableOcclusionCulling)
		{
			m_enableOcclusionCulling = enableOcclusionCulling;
		}
		
		inline bool IsOcclusionCullingEnabled()
		{
			return m_enableOcclusionCulling;
		}
		
		inline void CaptureVisibilityGraph()
		{
			m_shouldCaptureVisibilityGraph = true;
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
		
		static constexpr VkFormat ColorAttachmentFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
		
	private:
		static VkRenderPass CreateRenderPass();
		
		ChunkVisibilityCalculator m_visibilityCalculator;
		
		VkHandle<VkRenderPass> m_renderPass;
		
		RenderSettingsBuffer m_renderSettingsBuffer;
		
		CascadedShadowMapper m_shadowMapper;
		
		BlockShader m_blockShader;
		DebugShader m_debugShader;
		
		std::unique_ptr<ChunkVisibilityGraph> m_visibilityGraph;
		bool m_shouldCaptureVisibilityGraph = false;
		
		ChunkRenderList m_chunkRenderList;
		ChunkRenderList m_shadowRenderList;
		
		const Framebuffer* m_framebuffer;
		
		std::vector<CommandBuffer> m_commandBuffers;
		
		bool m_enableOcclusionCulling = true;
		
		bool m_isFrustumFrozen = false;
		Frustum m_frustum;
		
		bool m_wireframe = false;
		
		glm::mat4 m_projectionMatrix;
		glm::mat4 m_invProjectionMatrix;
		
		const WorldManager* m_worldManager = nullptr;
	};
}
