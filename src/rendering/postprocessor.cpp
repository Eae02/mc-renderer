#include "postprocessor.h"
#include "shaders/shadermodules.h"
#include "framebuffer.h"
#include "../vulkan/vkutils.h"
#include "../vulkan/instance.h"

#include <chrono>
#include <iostream>

namespace MCR
{
	static VkRenderPass CreateGodRaysRenderPass()
	{
		const VkAttachmentDescription attachment =
		{
			/* flags          */ 0,
			/* format         */ PostProcessor::GodRaysFormat,
			/* samples        */ VK_SAMPLE_COUNT_1_BIT,
			/* loadOp         */ VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			/* storeOp        */ VK_ATTACHMENT_STORE_OP_STORE,
			/* stencilLoadOp  */ VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			/* stencilStoreOp */ VK_ATTACHMENT_STORE_OP_DONT_CARE,
			/* initialLayout  */ VK_IMAGE_LAYOUT_UNDEFINED,
			/* finalLayout    */ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};
		
		VkAttachmentReference colorAttachmentRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		
		VkSubpassDescription subpass = { };
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		
		std::array<VkSubpassDependency, 1> dependencies;
		
		dependencies[0] = 
		{
			/* srcSubpass      */ 0,
			/* dstSubpass      */ VK_SUBPASS_EXTERNAL,
			/* srcStageMask    */ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			/* dstStageMask    */ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			/* srcAccessMask   */ VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			/* dstAccessMask   */ VK_ACCESS_SHADER_READ_BIT,
			/* dependencyFlags */ 0
		};
		
		VkRenderPassCreateInfo renderPassCreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		renderPassCreateInfo.attachmentCount = 1;
		renderPassCreateInfo.pAttachments = &attachment;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpass;
		renderPassCreateInfo.dependencyCount = dependencies.size();
		renderPassCreateInfo.pDependencies = dependencies.data();
		
		VkRenderPass renderPass;
		CheckResult(vkCreateRenderPass(vulkan.device, &renderPassCreateInfo, nullptr, &renderPass));
		return renderPass;
	}
	
	static VkRenderPass CreateSkyRenderPass()
	{
		const VkAttachmentDescription attachment =
		{
			/* flags          */ 0,
			/* format         */ SwapChain::GetImageFormat(),
			/* samples        */ VK_SAMPLE_COUNT_1_BIT,
			/* loadOp         */ VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			/* storeOp        */ VK_ATTACHMENT_STORE_OP_STORE,
			/* stencilLoadOp  */ VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			/* stencilStoreOp */ VK_ATTACHMENT_STORE_OP_STORE,
			/* initialLayout  */ VK_IMAGE_LAYOUT_UNDEFINED,
			/* finalLayout    */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};
		
		VkAttachmentReference colorAttachmentRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		
		VkSubpassDescription subpass = { };
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		
		VkRenderPassCreateInfo renderPassCreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		renderPassCreateInfo.attachmentCount = 1;
		renderPassCreateInfo.pAttachments = &attachment;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpass;
		
		VkRenderPass renderPass;
		CheckResult(vkCreateRenderPass(vulkan.device, &renderPassCreateInfo, nullptr, &renderPass));
		return renderPass;
	}
	
	constexpr uint32_t numQueriesPerFrame = 3;
	
	PostProcessor::PostProcessor(const VkDescriptorBufferInfo& renderSettingsBufferInfo)
	    : m_godRaysRenderPass(CreateGodRaysRenderPass()), m_skyRenderPass(CreateSkyRenderPass()),
	      m_timestampQueryPool(CreateQueryPool(VK_QUERY_TYPE_TIMESTAMP, SwapChain::GetImageCount() * numQueriesPerFrame)),
	      m_godRaysGenShader({ *m_godRaysRenderPass, 0 }, renderSettingsBufferInfo),
	      m_godRaysBlurShader({ *m_godRaysRenderPass, 0 }),
	      m_skyShader({ *m_skyRenderPass, 0 }, renderSettingsBufferInfo)
	{
		m_commandBuffers.reserve(SwapChain::GetImageCount());
		for (uint32_t i = 0; i < SwapChain::GetImageCount(); i++)
		{
			m_commandBuffers.emplace_back(vulkan.stdCommandPools[QUEUE_FAMILY_GRAPHICS]);
		}
	}
	
	void PostProcessor::FramebufferChanged(const Framebuffer& framebuffer)
	{
		m_skyShader.FramebufferChanged(framebuffer);
		m_godRaysGenShader.FramebufferChanged(framebuffer);
		m_godRaysBlurShader.FramebufferChanged(framebuffer);
		
		VkRect2D renderArea;
		VkViewport viewport;
		framebuffer.GetViewportAndRenderArea(renderArea, viewport);
		
		const VkRect2D godRaysGenRenderArea =
		{
			renderArea.offset,
			{ renderArea.extent.width / GodRaysDownscale, renderArea.extent.height / GodRaysDownscale }
		};
		
		const VkViewport godRaysGenViewport = 
		{
			0.0f, 0.0f,
			static_cast<float>(godRaysGenRenderArea.extent.width),
			static_cast<float>(godRaysGenRenderArea.extent.height),
			0.0f, 1.0f
		};
		
		const glm::vec2 pixelSize(1.0f / viewport.width, 1.0f / viewport.height);
		
		for (uint32_t i = 0; i < m_commandBuffers.size(); i++)
		{
			CommandBuffer& commandBuffer = m_commandBuffers[i];
			
			commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
			
			uint32_t firstTimestamp = i * numQueriesPerFrame;
			commandBuffer.WriteTimestamp(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, *m_timestampQueryPool, firstTimestamp);
			
			// ** God rays generate pass **
			const VkRenderPassBeginInfo godRaysGenRenderPassBeginInfo = 
			{
				/* sType           */ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				/* pNext           */ nullptr,
				/* renderPass      */ *m_godRaysRenderPass,
				/* framebuffer     */ framebuffer.GetGodRaysGenFramebuffer(),
				/* renderArea      */ godRaysGenRenderArea,
				/* clearValueCount */ 0,
				/* pClearValues    */ nullptr
			};
			commandBuffer.BeginRenderPass(&godRaysGenRenderPassBeginInfo);
			
			m_godRaysGenShader.Bind(commandBuffer);
			
			commandBuffer.SetViewport(0, SingleElementSpan(godRaysGenViewport));
			commandBuffer.SetScissor(0, SingleElementSpan(godRaysGenRenderArea));
			
			commandBuffer.Draw(3, 1, 0, 0);
			
			commandBuffer.EndRenderPass();
			
			// ** God rays horizontal blur pass **
			const VkRenderPassBeginInfo godRaysBlurRenderPassBeginInfo = 
			{
				/* sType           */ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				/* pNext           */ nullptr,
				/* renderPass      */ *m_godRaysRenderPass,
				/* framebuffer     */ framebuffer.GetGodRaysBlurFramebuffer(),
				/* renderArea      */ renderArea,
				/* clearValueCount */ 0,
				/* pClearValues    */ nullptr
			};
			commandBuffer.BeginRenderPass(&godRaysBlurRenderPassBeginInfo);
			
			m_godRaysBlurShader.Bind(commandBuffer);
			
			commandBuffer.SetViewport(0, SingleElementSpan(viewport));
			commandBuffer.SetScissor(0, SingleElementSpan(renderArea));
			
			commandBuffer.PushConstants(m_godRaysBlurShader.GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT,
			                            0, sizeof(float), &pixelSize.x);
			
			commandBuffer.Draw(3, 1, 0, 0);
			
			commandBuffer.EndRenderPass();
			
			commandBuffer.WriteTimestamp(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			                             *m_timestampQueryPool, firstTimestamp + 1);
			
			// ** Sky rendering pass **
			const VkRenderPassBeginInfo skyRenderPassBeginInfo = 
			{
				/* sType           */ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				/* pNext           */ nullptr,
				/* renderPass      */ *m_skyRenderPass,
				/* framebuffer     */ framebuffer.GetOutputFramebuffer(i),
				/* renderArea      */ renderArea,
				/* clearValueCount */ 0,
				/* pClearValues    */ nullptr
			};
			commandBuffer.BeginRenderPass(&skyRenderPassBeginInfo);
			
			m_skyShader.Bind(commandBuffer);
			
			commandBuffer.PushConstants(m_skyShader.GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT,
			                            0, sizeof(float), &pixelSize.y);
			
			commandBuffer.SetViewport(0, SingleElementSpan(viewport));
			commandBuffer.SetScissor(0, SingleElementSpan(renderArea));
			
			commandBuffer.Draw(3, 1, 0, 0);
			
			commandBuffer.EndRenderPass();
			
			commandBuffer.WriteTimestamp(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			                             *m_timestampQueryPool, firstTimestamp + 2);
			
			commandBuffer.End();
		}
	}
	
	PostProcessor::Timestamps PostProcessor::GetElapsedTime() const
	{
		uint64_t times[numQueriesPerFrame];
		VkResult result = vkGetQueryPoolResults(vulkan.device, *m_timestampQueryPool,
		                                        static_cast<uint32_t>(frameQueueIndex * numQueriesPerFrame),
		                                        numQueriesPerFrame, sizeof(times), times, sizeof(uint64_t),
		                                        VK_QUERY_RESULT_64_BIT);
		
		if (result == VK_NOT_READY)
			return { };
		
		CheckResult(result);
		
		return {
			std::chrono::duration<float, std::milli>((times[1] - times[0]) * vulkan.limits.timestampMillisecondPeriod),
			std::chrono::duration<float, std::milli>((times[2] - times[1]) * vulkan.limits.timestampMillisecondPeriod)
		};
	}
}
