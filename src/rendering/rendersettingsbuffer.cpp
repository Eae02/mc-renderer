#include "rendersettingsbuffer.h"
#include "viewprojection.h"
#include "../timemanager.h"

namespace MCR
{
	RenderSettingsBuffer::RenderSettingsBuffer()
	{
		// ** Creates the device buffer **
		const VmaAllocationCreateInfo deviceMemoryAllocationCI = 
		{
			0,
			VMA_MEMORY_USAGE_GPU_ONLY
		};
		
		VkBufferCreateInfo deviceBufferCreateInfo;
		InitBufferCreateInfo(deviceBufferCreateInfo, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
		                     VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(Data));
		
		CheckResult(vmaCreateBuffer(vulkan.allocator, &deviceBufferCreateInfo, &deviceMemoryAllocationCI,
		                            m_deviceBuffer.GetCreateAddress(), m_deviceAllocation.GetCreateAddress(), nullptr));
		
		// ** Creates the host buffer **
		const VmaAllocationCreateInfo hostMemoryAllocationCI = 
		{
			VMA_ALLOCATION_CREATE_MAPPED_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY
		};
		
		VkBufferCreateInfo hostBufferCreateInfo;
		InitBufferCreateInfo(hostBufferCreateInfo, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		                     sizeof(Data) * SwapChain::GetImageCount());
		
		VmaAllocationInfo hostAllocationInfo;
		
		CheckResult(vmaCreateBuffer(vulkan.allocator, &hostBufferCreateInfo, &hostMemoryAllocationCI,
		                            m_hostBuffer.GetCreateAddress(), m_hostAllocation.GetCreateAddress(),
		                            &hostAllocationInfo));
		
		m_data = reinterpret_cast<Data*>(hostAllocationInfo.pMappedData);
		
		m_postUploadBarrier = 
		{
			/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			/* pNext               */ nullptr,
			/* srcAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
			/* dstAccessMask       */ VK_ACCESS_UNIFORM_READ_BIT,
			/* srcQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
			/* dstQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
			/* buffer              */ *m_deviceBuffer,
			/* offset              */ 0,
			/* size                */ VK_WHOLE_SIZE
		};
		
		m_descriptorBufferInfo = 
		{
			/* buffer */ *m_deviceBuffer,
			/* offset */ 0,
			/* range  */ VK_WHOLE_SIZE
		};
	}
	
	glm::vec2 GetSunScreenPos(const ViewProjection& viewProj, const glm::vec3& toLight)
	{
		const glm::vec3 viewSpacePos = glm::normalize(glm::vec3(viewProj.m_view * glm::vec4(toLight, 0.0f)));
		const glm::vec4 ndcPos = viewProj.m_proj * glm::vec4(viewSpacePos, 0.0f);
		//invisible if ndcPos.z < 0
		
		return (glm::vec2(ndcPos) / ndcPos.w + glm::vec2(1.0f)) / 2.0f;
	}
	
	void RenderSettingsBuffer::SetData(CommandBuffer& cb, const ViewProjection& viewProj,
	                                   const glm::vec3& cameraPosition, float time, const TimeManager& timeManager)
	{
		m_data[frameQueueIndex].m_viewProj = viewProj.m_viewProj;
		m_data[frameQueueIndex].m_invViewProj = viewProj.m_invViewProj;
		m_data[frameQueueIndex].m_view = viewProj.m_view;
		m_data[frameQueueIndex].m_invView = viewProj.m_invView;
		m_data[frameQueueIndex].m_proj = viewProj.m_proj;
		m_data[frameQueueIndex].m_invProj = viewProj.m_invProj;
		m_data[frameQueueIndex].m_cameraPos = cameraPosition;
		m_data[frameQueueIndex].m_time = time;
		m_data[frameQueueIndex].m_sun = timeManager.GetSunDescription();
		m_data[frameQueueIndex].m_moon = timeManager.GetMoonDescription();
		m_data[frameQueueIndex].m_sunScreenPos = GetSunScreenPos(viewProj, m_data[frameQueueIndex].m_sun.m_direction);
		
		cb.CopyBuffer(*m_hostBuffer, *m_deviceBuffer, { sizeof(Data) * frameQueueIndex, 0, sizeof(Data) });
		
		cb.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0,
		                   { }, SingleElementSpan(m_postUploadBarrier), { });
	}
}
