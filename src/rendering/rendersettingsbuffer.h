#pragma once

#include "../vulkan/vk.h"

#include <glm/glm.hpp>

namespace MCR
{
	class RenderSettingsBuffer
	{
	public:
		RenderSettingsBuffer();
		
		inline const VkDescriptorBufferInfo& GetBufferInfo() const
		{
			return m_descriptorBufferInfo;
		}
		
		void SetData(CommandBuffer& cb, const glm::mat4& viewProj, const glm::vec3& cameraPosition, float time);
		
	private:
#pragma pack(push, 1)
		struct Data
		{
			glm::mat4 m_viewProj;
			glm::vec3 m_cameraPos;
			float m_time;
		};
#pragma pack(pop)
		
		VkHandle<VmaAllocation> m_hostAllocation;
		VkHandle<VmaAllocation> m_deviceAllocation;
		
		VkHandle<VkBuffer> m_hostBuffer;
		VkHandle<VkBuffer> m_deviceBuffer;
		
		VkBufferMemoryBarrier m_postUploadBarrier;
		
		VkDescriptorBufferInfo m_descriptorBufferInfo;
		
		Data* m_data;
	};
}