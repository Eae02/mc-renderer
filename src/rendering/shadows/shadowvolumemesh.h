#pragma once

#include "../../vulkan/vk.h"

namespace MCR
{
	class ShadowVolumeMesh
	{
	public:
		ShadowVolumeMesh(const class DirectionalShadowVolume& volume, CommandBuffer& commandBuffer);
		
		ShadowVolumeMesh(const glm::vec3* positions, CommandBuffer& commandBuffer);
		
		void Draw(CommandBuffer& commandBuffer, const class DebugShader& debugShader) const;
		
	private:
		VkHandle<VmaAllocation, VkHandleDestroyTime::Delayed> m_allocation;
		VkHandle<VkBuffer, VkHandleDestroyTime::Delayed> m_buffer;
	};
}
