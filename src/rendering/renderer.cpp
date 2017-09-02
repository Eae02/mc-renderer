#include "renderer.h"
#include "rendererframebuffer.h"

namespace MCR
{
	constexpr VkFormat Renderer::ColorAttachmentFormat;
	
	Renderer::Renderer()
	{
		for (CommandBuffer& cb : m_commandBuffers)
		{
			cb = CommandBuffer(vulkan.stdCommandPools[QUEUE_FAMILY_GRAPHICS]);
		}
		
		std::array<VkAttachmentDescription, 2> attachments = { };
		attachments[0].format = ColorAttachmentFormat;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		
		attachments[1].format = vulkan.depthFormat;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		
		VkAttachmentReference colorAttachmentRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkAttachmentReference depthStencilAttachmentRef = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
		
		std::array<VkSubpassDescription, 1> subpassDescriptions = { };
		subpassDescriptions[0].colorAttachmentCount = 1;
		subpassDescriptions[0].pColorAttachments = &colorAttachmentRef;
		subpassDescriptions[0].pDepthStencilAttachment = &depthStencilAttachmentRef;
		
		VkRenderPassCreateInfo renderPassCreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		renderPassCreateInfo.attachmentCount = attachments.size();
		renderPassCreateInfo.pAttachments = attachments.data();
		renderPassCreateInfo.subpassCount = subpassDescriptions.size();
		renderPassCreateInfo.pSubpasses = subpassDescriptions.data();
		
		CheckResult(vkCreateRenderPass(vulkan.device, &renderPassCreateInfo, nullptr, m_renderPass.GetCreateAddress()));
	}
	
	void Renderer::Render(const RenderParams& params)
	{
		CommandBuffer& cb = m_commandBuffers[frameQueueIndex];
		
		cb.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		
		const VkRect2D renderArea = { { }, { params.m_framebuffer->GetWidth(), params.m_framebuffer->GetHeight() } };
		
		std::array<VkClearValue, 2> clearValues;
		SetColorClearValue(clearValues[0], glm::vec4 { 0.0f, 0.0f, 0.0f, 0.0f });
		SetDepthStencilClearValue(clearValues[1], 1.0f, 0);
		
		VkRenderPassBeginInfo renderPassBeginInfo = 
		{
		    /* sType           */ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		    /* pNext           */ nullptr,
		    /* renderPass      */ *m_renderPass,
		    /* framebuffer     */ params.m_framebuffer->GetFramebuffer(),
		    /* renderArea      */ renderArea,
		    /* clearValueCount */ clearValues.size(),
		    /* pClearValues    */ clearValues.data()
		};
		
		cb.BeginRenderPass(&renderPassBeginInfo);
		
		cb.EndRenderPass();
		
		cb.End();
		
		const VkSubmitInfo submitInfo = 
		{
			/* sType                */ VK_STRUCTURE_TYPE_SUBMIT_INFO,
			/* pNext                */ nullptr,
			/* waitSemaphoreCount   */ 0,
			/* pWaitSemaphores      */ nullptr,
			/* pWaitDstStageMask    */ nullptr,
			/* commandBufferCount   */ 1,
			/* pCommandBuffers      */ &cb.GetVkCB(),
			/* signalSemaphoreCount */ 1,
			/* pSignalSemaphores    */ &params.m_signalSemaphore
		};
		
		vulkan.queues[QUEUE_FAMILY_GRAPHICS]->Submit(1, &submitInfo, params.m_signalFence);
	}
}
