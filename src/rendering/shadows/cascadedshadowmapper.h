#pragma once

#include "../../vulkan/vk.h"
#include "../shaders/shader.h"
#include "../viewprojection.h"
#include "../constants.h"
#include "../chunkvisibilitycalculator.h"
#include "../chunkrenderlist.h"
#include "directionalshadowvolume.h"

namespace MCR
{
	class CascadedShadowMapper
	{
	public:
		CascadedShadowMapper();
		
		void SetResolution(uint32_t resolution);
		
		void CalculateSlices(const glm::vec3& lightDirection, const ViewProjection& viewProjection,
		                     const class WorldManager& worldManager);
		
		void Render(CommandBuffer& commandBuffer, const ChunkRenderList& shadowRenderList);
		
		inline VkDescriptorSet GetDescriptorSet() const
		{ return *m_sampleDescriptorSet; }
		
		inline const DirectionalShadowVolume& GetShadowVolume() const
		{ return m_shadowVolume; }
		
	private:
		struct FrustumSlice
		{
			//The end depth for this slice (in post projection space)
			float m_endDepth;
			
			//The vertices for this slice (in world space)
			glm::vec3 m_vertices[8];
			
			//The center of this slice (in world space)
			glm::vec3 m_center;
		};
		
		void CalcFrustumSlices(const ViewProjection& viewProj, float shadowEndDist);
		glm::mat4 GetCascadeProjectionMatrix(const glm::mat4& sliceMatrix, const FrustumSlice& slice);
		glm::mat4 GetTexelAlignMatrix(const glm::mat4& lightMatrix);
		
		VkHandle<VkRenderPass> m_renderPass;
		
		Shader m_shader;
		
		uint32_t m_resolution;
		
		VkHandle<VkImage, VkHandleDestroyTime::Delayed> m_shadowMap;
		VkHandle<VmaAllocation, VkHandleDestroyTime::Delayed> m_shadowMapAllocation;
		VkHandle<VkImageView, VkHandleDestroyTime::Delayed> m_shadowMapView;
		VkHandle<VkFramebuffer, VkHandleDestroyTime::Delayed> m_framebuffer;
		
#pragma pack(push, 1)
		struct ShadowInfo
		{
			glm::mat4 m_lightMatrices[DirLightCascades];
			glm::vec4 m_sliceEndDepths[DirLightCascades];
		};
#pragma pack(pop)
		
		VkHandle<VmaAllocation> m_infoHostAllocation;
		VkHandle<VkBuffer> m_infoHostBuffer;
		ShadowInfo* m_shadowInfoMemory;
		
		VkHandle<VmaAllocation> m_infoDeviceAllocation;
		VkHandle<VkBuffer> m_infoDeviceBuffer;
		
		UniqueDescriptorSet m_renderDescriptorSet;
		UniqueDescriptorSet m_sampleDescriptorSet;
		
		DirectionalShadowVolume m_shadowVolume;
		
		std::array<FrustumSlice, DirLightCascades> m_frustumSlices;
	};
}
