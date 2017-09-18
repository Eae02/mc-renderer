#include "rendersettingsbuffer.h"

namespace MCR
{
	RenderSettingsBuffer::RenderSettingsBuffer()
	{
		// ** Creates the device buffer **
		const VmaMemoryRequirements deviceMemoryRequirements = 
		{
			0,
			VMA_MEMORY_USAGE_GPU_ONLY
		};
		
		VkBufferCreateInfo deviceBufferCreateInfo;
		InitBufferCreateInfo(deviceBufferCreateInfo, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
		                     VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(Data));
		
		CheckResult(vmaCreateBuffer(vulkan.allocator, &deviceBufferCreateInfo, &deviceMemoryRequirements,
		                            m_deviceBuffer.GetCreateAddress(), m_deviceAllocation.GetCreateAddress(), nullptr));
		
		// ** Creates the host buffer **
		const VmaMemoryRequirements hostMemoryRequirements = 
		{
			VMA_MEMORY_REQUIREMENT_PERSISTENT_MAP_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY
		};
		
		VkBufferCreateInfo hostBufferCreateInfo;
		InitBufferCreateInfo(hostBufferCreateInfo, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, sizeof(Data) * MaxQueuedFrames);
		
		VmaAllocationInfo hostAllocationInfo;
		
		CheckResult(vmaCreateBuffer(vulkan.allocator, &hostBufferCreateInfo, &hostMemoryRequirements,
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
	
	void RenderSettingsBuffer::SetData(CommandBuffer& cb, const glm::mat4& viewProj, const glm::mat4& invViewProj,
	                                   const glm::vec3& cameraPosition, float time)
	{
		m_data[frameQueueIndex].m_viewProj = viewProj;
		m_data[frameQueueIndex].m_invViewProj = invViewProj;
		m_data[frameQueueIndex].m_cameraPos = cameraPosition;
		m_data[frameQueueIndex].m_time = time;
		
		cb.CopyBuffer(*m_hostBuffer, *m_deviceBuffer, { sizeof(Data) * frameQueueIndex, 0, sizeof(Data) });
		
		cb.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0,
		                   { }, SingleElementSpan(m_postUploadBarrier), { });
	}
}
