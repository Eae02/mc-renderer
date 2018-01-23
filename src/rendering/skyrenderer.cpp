#include "skyrenderer.h"
#include "shaders/shadermodules.h"
#include "framebuffer.h"
#include "../vulkan/vkutils.h"
#include "../vulkan/instance.h"
#include "renderer.h"
#include "fbformats.h"
#include "../profiling/profiling.h"

#include <chrono>
#include <iostream>

namespace MCR
{
	static VkRenderPass CreateGodRaysRenderPass()
	{
		const VkAttachmentDescription attachment =
		{
			/* flags          */ 0,
			/* format         */ FBFormats::GodRays,
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
			/* format         */ FBFormats::HDRColor,
			/* samples        */ VK_SAMPLE_COUNT_1_BIT,
			/* loadOp         */ VK_ATTACHMENT_LOAD_OP_LOAD,
			/* storeOp        */ VK_ATTACHMENT_STORE_OP_STORE,
			/* stencilLoadOp  */ VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			/* stencilStoreOp */ VK_ATTACHMENT_STORE_OP_DONT_CARE,
			/* initialLayout  */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, //Image was just drawn to by the main render pass
			/* finalLayout    */ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL //Image will be sampled by the water shader
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
	
	SkyRenderer::SkyRenderer(const VkDescriptorBufferInfo& renderSettingsBufferInfo)
	    : m_godRaysRenderPass(CreateGodRaysRenderPass()), m_skyRenderPass(CreateSkyRenderPass()),
	      m_godRaysGenShader({ *m_godRaysRenderPass, 0 }, renderSettingsBufferInfo),
	      m_godRaysBlurShader({ *m_godRaysRenderPass, 0 }),
	      m_skyShader({ *m_skyRenderPass, 0 }, renderSettingsBufferInfo),
	      m_starRenderer({ *m_skyRenderPass, 0 }, renderSettingsBufferInfo)
	{
		
	}
	
	void SkyRenderer::FramebufferChanged(const Framebuffer& framebuffer)
	{
		m_skyShader.FramebufferChanged(framebuffer);
		m_godRaysGenShader.FramebufferChanged(framebuffer);
		m_godRaysBlurShader.FramebufferChanged(framebuffer);
		m_starRenderer.FramebufferChanged(framebuffer);
	}
	
	void SkyRenderer::Render(CommandBuffer& commandBuffer, const Framebuffer& framebuffer,
	                         const class TimeManager& timeManager)
	{
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
		
		uint32_t godRaysTimer = BeginGPUTimer(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, "God Rays");
		
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
		
		EndGPUTimer(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, godRaysTimer);
		
		uint32_t skyTimer = BeginGPUTimer(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, "Sky");
		
		// ** Sky rendering pass **
		const VkRenderPassBeginInfo skyRenderPassBeginInfo =
		{
			/* sType           */ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			/* pNext           */ nullptr,
			/* renderPass      */ *m_skyRenderPass,
			/* framebuffer     */ framebuffer.GetSkyFramebuffer(),
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
		
		m_starRenderer.Render(commandBuffer, timeManager);
		
		commandBuffer.EndRenderPass();
		
		EndGPUTimer(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, skyTimer);
	}
}
