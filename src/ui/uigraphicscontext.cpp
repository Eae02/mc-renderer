#include "uigraphicscontext.h"
#include "uidrawlist.h"
#include "../profiling/profiling.h"
#include "../rendering/framebuffer.h"

namespace MCR
{
	static VkRenderPass CreateRenderPass()
	{
		const VkAttachmentDescription attachment = 
		{
			/* flags          */ 0,
			/* format         */ SwapChain::GetImageFormat(),
			/* samples        */ VK_SAMPLE_COUNT_1_BIT,
			/* loadOp         */ VK_ATTACHMENT_LOAD_OP_LOAD,
			/* storeOp        */ VK_ATTACHMENT_STORE_OP_STORE,
			/* stencilLoadOp  */ VK_ATTACHMENT_LOAD_OP_LOAD,
			/* stencilStoreOp */ VK_ATTACHMENT_STORE_OP_STORE,
			/* initialLayout  */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			/* finalLayout    */ VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		};
		
		VkAttachmentReference attachmentRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		
		VkSubpassDescription subpass = { };
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &attachmentRef;
		
		std::array<VkSubpassDependency, 2> subpassDependencies = { };
		
		subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[0].dstSubpass = 0;
		subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		
		subpassDependencies[1].srcSubpass = 0;
		subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		
		VkRenderPassCreateInfo renderPassCreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		renderPassCreateInfo.attachmentCount = 1;
		renderPassCreateInfo.pAttachments = &attachment;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpass;
		renderPassCreateInfo.dependencyCount = subpassDependencies.size();
		renderPassCreateInfo.pDependencies = subpassDependencies.data();
		
		VkRenderPass renderPass;
		CheckResult(vkCreateRenderPass(vulkan.device, &renderPassCreateInfo, nullptr, &renderPass));
		return renderPass;
	}
	
	UIGraphicsContext::UIGraphicsContext()
	    : m_renderPass(CreateRenderPass()), m_samplerDescriptorSet("UI_Sampler"), m_shader(*m_renderPass)
	{
		m_commandBuffers.reserve(SwapChain::GetImageCount());
		for (uint32_t i = 0; i < SwapChain::GetImageCount(); i++)
		{
			m_commandBuffers.emplace_back(vulkan.stdCommandPools[QUEUE_FAMILY_GRAPHICS]);
		}
		
		const VkSamplerCreateInfo samplerCreateInfo = 
		{
			/* sType                   */ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			/* pNext                   */ nullptr,
			/* flags                   */ 0,
			/* magFilter               */ VK_FILTER_LINEAR,
			/* minFilter               */ VK_FILTER_LINEAR,
			/* mipmapMode              */ VK_SAMPLER_MIPMAP_MODE_LINEAR,
			/* addressModeU            */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			/* addressModeV            */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			/* addressModeW            */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			/* mipLodBias              */ 0.0f,
			/* anisotropyEnable        */ VK_FALSE,
			/* maxAnisotropy           */ 0,
			/* compareEnable           */ VK_FALSE,
			/* compareOp               */ VK_COMPARE_OP_ALWAYS,
			/* minLod                  */ 0.0f,
			/* maxLod                  */ 0.0f,
			/* borderColor             */ VK_BORDER_COLOR_INT_OPAQUE_WHITE,
			/* unnormalizedCoordinates */ VK_FALSE
		};
		
		CheckResult(vkCreateSampler(vulkan.device, &samplerCreateInfo, nullptr, m_sampler.GetCreateAddress()));
		
		VkWriteDescriptorSet samplerWrite;
		const VkDescriptorImageInfo samplerDescriptorInfo = { *m_sampler, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED };
		m_samplerDescriptorSet.InitWriteDescriptorSet(samplerWrite, 0, VK_DESCRIPTOR_TYPE_SAMPLER, samplerDescriptorInfo);
		UpdateDescriptorSets(SingleElementSpan(samplerWrite));
		
		VkImageCreateInfo defaultImageCreateInfo;
		InitImageCreateInfo(defaultImageCreateInfo, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 1, 1);
		
	}
	
	void UIGraphicsContext::Draw(const UIDrawList& drawList, const Framebuffer& framebuffer)
	{
		CommandBuffer& commandBuffer = m_commandBuffers[frameQueueIndex];
		
		commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		
		if (!drawList.Empty())
		{
			//Reallocates the vertex & index buffers if they're not large enough.
			const uint64_t reqBufferSize = drawList.GetRequiredBufferSize();
			const uint64_t minBufferSize = RoundToNextMultiple<uint64_t>(reqBufferSize, 1024);
			if (m_bufferSize < minBufferSize)
			{
				vulkan.queues[QUEUE_FAMILY_GRAPHICS]->WaitIdle();
				
				// ** Allocates the host buffer **
				VkBufferCreateInfo hostBufferCreateInfo;
				InitBufferCreateInfo(hostBufferCreateInfo, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				                     minBufferSize * SwapChain::GetImageCount());
				
				const VmaMemoryRequirements hostBufferMemRequirements = 
				{
					VMA_MEMORY_REQUIREMENT_PERSISTENT_MAP_BIT,
					VMA_MEMORY_USAGE_CPU_ONLY
				};
				
				VmaAllocationInfo hostAllocationInfo;
				
				CheckResult(vmaCreateBuffer(vulkan.allocator, &hostBufferCreateInfo, &hostBufferMemRequirements,
				                            m_hostBuffer.GetCreateAddress(), m_hostBufferAllocation.GetCreateAddress(),
				                            &hostAllocationInfo));
				
				for (uint32_t i = 0; i < SwapChain::GetImageCount(); i++)
				{
					m_hostBufferMemory[i] = reinterpret_cast<char*>(hostAllocationInfo.pMappedData) + i * minBufferSize;
				}
				
				// ** Allocates the device buffer **
				VkBufferCreateInfo deviceBufferCreateInfo;
				InitBufferCreateInfo(deviceBufferCreateInfo, VK_BUFFER_USAGE_TRANSFER_DST_BIT |
				                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, minBufferSize);
				
				const VmaMemoryRequirements deviceBufferMemRequirements = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
				
				CheckResult(vmaCreateBuffer(vulkan.allocator, &deviceBufferCreateInfo, &deviceBufferMemRequirements,
				                            m_deviceBuffer.GetCreateAddress(),
				                            m_deviceBufferAllocation.GetCreateAddress(), nullptr));
				
				m_bufferSize = minBufferSize;
			}
			
			drawList.CopyToBuffer(m_hostBufferMemory[frameQueueIndex]);
			
			const VkBufferCopy bufferCopyRegion = { frameQueueIndex * m_bufferSize, 0, reqBufferSize };
			commandBuffer.CopyBuffer(*m_hostBuffer, *m_deviceBuffer, bufferCopyRegion);
			
			const VkBufferMemoryBarrier bufferBarrier = 
			{
				/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				/* pNext               */ nullptr,
				/* srcAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
				/* dstAccessMask       */ VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT,
				/* srcQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
				/* dstQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
				/* buffer              */ *m_deviceBuffer,
				/* offset              */ 0,
				/* size                */ reqBufferSize
			};
			
			commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
			                              { }, SingleElementSpan(bufferBarrier), { });
		}
		
		if (!m_defaultImage)
		{
			m_defaultImage = std::make_unique<UIImage>(UIImage::CreateSingleColor(glm::vec4(1.0f), 1, 1, commandBuffer));
		}
		
		VkRect2D renderArea;
		VkViewport viewport;
		framebuffer.GetViewportAndRenderArea(renderArea, viewport);
		
		const VkRenderPassBeginInfo renderPassBeginInfo = 
		{
			/* sType           */ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			/* pNext           */ nullptr,
			/* renderPass      */ *m_renderPass,
			/* framebuffer     */ framebuffer.GetOutputFramebuffer(frameQueueIndex),
			/* renderArea      */ renderArea,
			/* clearValueCount */ 0,
			/* pClearValues    */ nullptr
		};
		commandBuffer.BeginRenderPass(&renderPassBeginInfo);
		
		if (!drawList.Empty())
		{
			m_shader.Bind(commandBuffer, Shader::BindModes::Default);
			
			commandBuffer.SetViewport(0, SingleElementSpan(viewport));
			commandBuffer.SetScissor(0, SingleElementSpan(renderArea));
			
			VkDescriptorSet samplerDescriptorSet = *m_samplerDescriptorSet;
			commandBuffer.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_shader.GetLayout(), 0,
			                                 SingleElementSpan(samplerDescriptorSet));
			
			const glm::vec2 positionScale(2.0f / static_cast<float>(framebuffer.GetWidth()),
			                              2.0f / static_cast<float>(framebuffer.GetHeight()));
			commandBuffer.PushConstants(m_shader.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0,
			                            sizeof(positionScale), &positionScale);
			
			drawList.Draw(commandBuffer, m_shader.GetLayout(), m_defaultImage->GetDescriptorSet(), *m_deviceBuffer);
		}
		
		commandBuffer.EndRenderPass();
		
		commandBuffer.End();
	}
}
