#include "postprocessor.h"
#include "framebuffer.h"

namespace MCR
{
	static VkRenderPass CreateRenderPass()
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
	
	PostProcessor::PostProcessor()
	    : m_finalRenderPass(CreateRenderPass()), m_postProcessShader({ *m_finalRenderPass, 0 }) 
	{
		m_commandBuffers.reserve(SwapChain::GetImageCount());
		for (uint32_t i = 0; i < SwapChain::GetImageCount(); i++)
		{
			m_commandBuffers.emplace_back(vulkan.stdCommandPools[QUEUE_FAMILY_GRAPHICS]);
		}
	}
	
	void PostProcessor::FramebufferChanged(const Framebuffer& framebuffer)
	{
		m_postProcessShader.FramebufferChanged(framebuffer);
		
		VkRect2D renderArea;
		VkViewport viewport;
		framebuffer.GetViewportAndRenderArea(renderArea, viewport);
		
		for (uint32_t i = 0; i < m_commandBuffers.size(); i++)
		{
			CommandBuffer& commandBuffer = m_commandBuffers[i];
			
			commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
			
			// ** Post process pass **
			const VkRenderPassBeginInfo godRaysGenRenderPassBeginInfo =
			{
				/* sType           */ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				/* pNext           */ nullptr,
				/* renderPass      */ *m_finalRenderPass,
				/* framebuffer     */ framebuffer.GetOutputFramebuffer(i),
				/* renderArea      */ renderArea,
				/* clearValueCount */ 0,
				/* pClearValues    */ nullptr
			};
			commandBuffer.BeginRenderPass(&godRaysGenRenderPassBeginInfo);
			
			m_postProcessShader.Bind(commandBuffer);
			
			commandBuffer.SetViewport(0, SingleElementSpan(viewport));
			commandBuffer.SetScissor(0, SingleElementSpan(renderArea));
			
			commandBuffer.Draw(3, 1, 0, 0);
			
			commandBuffer.EndRenderPass();
			
			commandBuffer.End();
		}
	}
}
