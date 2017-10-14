#include "shadowvolumemesh.h"
#include "directionalshadowvolume.h"
#include "../shaders/debugshader.h"

namespace MCR
{
	static const uint32_t frustumIndices[] = 
	{
		0, 1, 2,  0, 2, 3,
		4, 5, 6,  4, 6, 7,
		
		0, 1, 4,  1, 4, 5,
		2, 3, 6,  3, 6, 7,
		
		0, 3, 4,  3, 4, 7,
		1, 2, 5,  2, 5, 6
	};
	
	static const size_t numPositions = 8 * DirLightCascades;
	static const size_t numIndices = ArrayLength(frustumIndices) * DirLightCascades;
	static const uint64_t bufferSize = numIndices * sizeof(uint32_t) + sizeof(glm::vec3) * numPositions;
	
	ShadowVolumeMesh::ShadowVolumeMesh(const DirectionalShadowVolume& volume, CommandBuffer& commandBuffer)
	{
		VkBuffer hostBuffer;
		VmaAllocation hostAllocation;
		void* hostMemory;
		
		CreateStagingBuffer(bufferSize, &hostBuffer, &hostAllocation, &hostMemory);
		
		// ** Fills the staging buffer **
		uint32_t* indices = reinterpret_cast<uint32_t*>(hostMemory);
		glm::vec3* positions = reinterpret_cast<glm::vec3*>(indices + numIndices);
		
		for (uint32_t i = 0; i < DirLightCascades; i++)
		{
			for (size_t v = 0; v < 8; v++)
			{
				*(positions++) = volume.GetFrustum(i).GetCorner(v);
			}
			
			const uint32_t baseIndex = i * 8;
			for (uint32_t index : frustumIndices)
			{
				*(indices++) = baseIndex + index;
			}
		}
		
		// ** Allocates the device buffer **
		VmaMemoryRequirements memoryRequirements = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
		VkBufferCreateInfo bufferCreateInfo;
		InitBufferCreateInfo(bufferCreateInfo, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
		                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize);
		
		CheckResult(vmaCreateBuffer(vulkan.allocator, &bufferCreateInfo, &memoryRequirements,
		                            m_buffer.GetCreateAddress(), m_allocation.GetCreateAddress(), nullptr));
		
		// ** Copies data to the device buffer **
		commandBuffer.CopyBuffer(hostBuffer, *m_buffer, { 0, 0, bufferSize });
		
		const VkBufferMemoryBarrier barrier = 
		{
			/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			/* pNext               */ nullptr,
			/* srcAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
			/* dstAccessMask       */ VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT,
			/* srcQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
			/* dstQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
			/* buffer              */ *m_buffer,
			/* offset              */ 0,
			/* size                */ VK_WHOLE_SIZE
		};
		
		commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
		                              { }, SingleElementSpan(barrier), { });
		
		DelayedDestroyVulkanObject(hostBuffer);
		DelayedDestroyVulkanObject(hostAllocation);
	}
	
	ShadowVolumeMesh::ShadowVolumeMesh(const glm::vec3* positions, CommandBuffer& commandBuffer)
	{
		VkBuffer hostBuffer;
		VmaAllocation hostAllocation;
		void* hostMemory;
		
		CreateStagingBuffer(bufferSize, &hostBuffer, &hostAllocation, &hostMemory);
		
		// ** Fills the staging buffer **
		uint32_t* indices = reinterpret_cast<uint32_t*>(hostMemory);
		std::copy_n(positions, DirLightCascades * 8, reinterpret_cast<glm::vec3*>(indices + numIndices));
		
		for (uint32_t i = 0; i < DirLightCascades; i++)
		{
			const uint32_t baseIndex = i * 8;
			for (uint32_t index : frustumIndices)
			{
				*(indices++) = baseIndex + index;
			}
		}
		
		// ** Allocates the device buffer **
		VmaMemoryRequirements memoryRequirements = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
		VkBufferCreateInfo bufferCreateInfo;
		InitBufferCreateInfo(bufferCreateInfo, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
		                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize);
		
		CheckResult(vmaCreateBuffer(vulkan.allocator, &bufferCreateInfo, &memoryRequirements,
		                            m_buffer.GetCreateAddress(), m_allocation.GetCreateAddress(), nullptr));
		
		// ** Copies data to the device buffer **
		commandBuffer.CopyBuffer(hostBuffer, *m_buffer, { 0, 0, bufferSize });
		
		const VkBufferMemoryBarrier barrier = 
		{
			/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			/* pNext               */ nullptr,
			/* srcAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
			/* dstAccessMask       */ VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT,
			/* srcQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
			/* dstQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
			/* buffer              */ *m_buffer,
			/* offset              */ 0,
			/* size                */ VK_WHOLE_SIZE
		};
		
		commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
		                              { }, SingleElementSpan(barrier), { });
		
		DelayedDestroyVulkanObject(hostBuffer);
		DelayedDestroyVulkanObject(hostAllocation);
	}
	
	void ShadowVolumeMesh::Draw(CommandBuffer& commandBuffer, const DebugShader& debugShader) const
	{
		const glm::vec3 colors[] =
		{
			glm::vec3(1.0f, 0.3f, 0.3f),
			glm::vec3(0.3f, 1.0f, 0.3f),
			glm::vec3(0.3f, 0.3f, 1.0f)
		};
		
		VkDeviceSize offset[] = { numIndices * sizeof(uint32_t) };
		commandBuffer.BindVertexBuffers(0, 1, &*m_buffer, offset);
		
		commandBuffer.BindIndexBuffer(*m_buffer, 0, VK_INDEX_TYPE_UINT32);
		
		for (int m = 0; m < 2; m++)
		{
			debugShader.Bind(commandBuffer, m == 0 ? Shader::BindModes::Default : Shader::BindModes::Wireframe);
			
			const float opacity = m == 0 ? 0.3f : 1.0f;
			
			for (uint32_t i = 0; i < DirLightCascades; i++)
			{
				debugShader.SetColor(commandBuffer, glm::vec4(colors[i], opacity));
				commandBuffer.DrawIndexed(ArrayLength(frustumIndices), 1, ArrayLength(frustumIndices) * i, 0, 0);
			}
		}
	}
}
