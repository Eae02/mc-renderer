#pragma once

#include "../vulkan/vk.h"
#include "frustum.h"
#include "chunkrenderlist.h"
#include "framebuffer.h"
#include "rendersettingsbuffer.h"
#include "shaders/blockshader.h"
#include "shaders/debugshader.h"
#include "skyrenderer.h"
#include "chunkvisibilitycalculator.h"
#include "shadows/cascadedshadowmapper.h"
#include "shadows/shadowvolumemesh.h"
#include "shaders/water/watershader.h"
#include "shaders/water/waterpostshader.h"
#include "causticstexture.h"
#include "starrenderer.h"

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
		
		void Initialize(class LoadContext& loadContext);
		
		void Render(const RenderParams& params);
		
		void SaveCausticsTexture() const;
		
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
		
		inline void SetWireframeWater(bool wireframeWater)
		{
			m_wireframeWater = wireframeWater;
		}
		
		inline bool WireframeWater() const
		{
			return m_wireframeWater;
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
		
		inline void CaptureShadowVolume()
		{
			m_shouldCaptureShadowVolume = true;
		}
		
		inline void ClearVisibilityGraph()
		{
			m_visibilityGraph.reset();
		}
		
		inline void ClearShadowVolume()
		{
			m_shadowVolume.reset();
		}
		
		inline void GetRenderPasses(Framebuffer::RenderPasses& renderPasses) const
		{
			renderPasses.m_renderer = *m_renderPass;
			renderPasses.m_water = *m_waterRenderPass;
			renderPasses.m_godRays = m_skyRenderer.GetGodRaysRenderPass();
			renderPasses.m_sky = m_skyRenderer.GetSkyRenderPass();
		}
		
		inline const VkDescriptorBufferInfo& GetRenderSettingsBufferInfo() const
		{
			return m_renderSettingsBuffer.GetBufferInfo();
		}
		
		inline void SetWorldManager(WorldManager* worldManager)
		{
			m_worldManager = worldManager;
		}
		
		inline void SetShadowQualityLevel(QualityLevels qualityLevel)
		{
			m_shadowMapper.SetQualityLevel(qualityLevel);
		}
		
		void FramebufferChanged(const Framebuffer& framebuffer);
		
	private:
		static VkRenderPass CreateRenderPass();
		static VkRenderPass CreateWaterRenderPass();
		
		ChunkVisibilityCalculator m_visibilityCalculator;
		
		VkHandle<VkRenderPass> m_renderPass;
		VkHandle<VkRenderPass> m_waterRenderPass;
		
		RenderSettingsBuffer m_renderSettingsBuffer;
		
		CascadedShadowMapper m_shadowMapper;
		
		CausticsTexture m_causticsTexture;
		
		BlockShader m_blockShader;
		WaterShader m_waterShader;
		WaterPostShader m_waterPostShader;
		DebugShader m_debugShader;
		
		SkyRenderer m_skyRenderer;
		
		bool m_shouldCaptureShadowVolume = false;
		std::unique_ptr<ShadowVolumeMesh> m_shadowVolume;
		
		std::unique_ptr<ShadowVolumeMesh> m_frustumVolume;
		
		std::unique_ptr<ChunkVisibilityGraph> m_visibilityGraph;
		bool m_shouldCaptureVisibilityGraph = false;
		
		ChunkRenderList m_chunkRenderList;
		ChunkRenderList m_shadowRenderList;
		
		const Framebuffer* m_framebuffer;
		
		std::vector<CommandBuffer> m_commandBuffers;
		
		bool m_enableOcclusionCulling = true;
		
		bool m_isFrustumFrozen = false;
		Frustum m_frustum;
		
		bool m_wireframeWater = false;
		bool m_wireframe = false;
		
		glm::mat4 m_projectionMatrix;
		glm::mat4 m_invProjectionMatrix;
		
		WorldManager* m_worldManager = nullptr;
	};
}
