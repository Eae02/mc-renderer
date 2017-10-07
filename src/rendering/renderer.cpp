#include "renderer.h"
#include "frustum.h"
#include "framebuffer.h"
#include "regions/chunkbufferallocator.h"
#include "../profiling/profiling.h"
#include "../world/worldmanager.h"
#include "../profiling/frameprofiler.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

namespace MCR
{
	constexpr VkFormat Renderer::ColorAttachmentFormat;
	
	VkRenderPass Renderer::CreateRenderPass()
	{
		std::array<VkAttachmentDescription, 2> attachments = { };
		attachments[0].format = ColorAttachmentFormat;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		
		attachments[1].format = vulkan.depthFormat;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		
		VkAttachmentReference colorAttachmentRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkAttachmentReference depthStencilAttachmentRef = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
		
		std::array<VkSubpassDescription, 1> subpassDescriptions = { };
		subpassDescriptions[0].colorAttachmentCount = 1;
		subpassDescriptions[0].pColorAttachments = &colorAttachmentRef;
		subpassDescriptions[0].pDepthStencilAttachment = &depthStencilAttachmentRef;
		
		std::array<VkSubpassDependency, 1> dependencies;
		dependencies[0].srcSubpass = 0;
		dependencies[0].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		
		VkRenderPassCreateInfo renderPassCreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		renderPassCreateInfo.attachmentCount = attachments.size();
		renderPassCreateInfo.pAttachments = attachments.data();
		renderPassCreateInfo.subpassCount = subpassDescriptions.size();
		renderPassCreateInfo.pSubpasses = subpassDescriptions.data();
		renderPassCreateInfo.dependencyCount = dependencies.size();
		renderPassCreateInfo.pDependencies = dependencies.data();
		
		VkRenderPass renderPass;
		CheckResult(vkCreateRenderPass(vulkan.device, &renderPassCreateInfo, nullptr, &renderPass));
		return renderPass;
	}
	
	Renderer::Renderer()
	    : m_renderPass(CreateRenderPass()),
	      m_blockShader(*m_renderPass, m_renderSettingsBuffer.GetBufferInfo()),
	      m_debugShader(*m_renderPass, m_renderSettingsBuffer.GetBufferInfo())
	{
		m_commandBuffers.reserve(SwapChain::GetImageCount());
		for (uint32_t i = 0; i < SwapChain::GetImageCount(); i++)
		{
			m_commandBuffers.emplace_back(vulkan.stdCommandPools[QUEUE_FAMILY_GRAPHICS]);
		}
	}
	
	void Renderer::Render(const RenderParams& params)
	{
		CommandBuffer& cb = m_commandBuffers[frameQueueIndex];
		
		cb.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		
		const Camera& camera = m_worldManager->GetCamera();
		
		const glm::mat4 viewProj = m_projectionMatrix * camera.GetViewMatrix();
		const glm::mat4 invViewProj = camera.GetInverseViewMatrix() * m_invProjectionMatrix;
		
#ifdef MCR_DEBUG
		currentFrameProfiler->Reset(cb);
#endif
		
		m_renderSettingsBuffer.SetData(cb, viewProj, invViewProj, camera.GetPosition(), params.m_time,
		                               *params.m_timeManager);
		
		if (!m_isFrustumFrozen)
		{
			m_frustum = Frustum(invViewProj);
		}
		
		VkRect2D renderArea;
		VkViewport viewport;
		m_framebuffer->GetViewportAndRenderArea(renderArea, viewport);
		
		std::array<VkClearValue, 2> clearValues;
		SetColorClearValue(clearValues[0], glm::vec4 { 0.0f, 0.0f, 0.0f, 1.0f });
		SetDepthStencilClearValue(clearValues[1], 1.0f, 0);
		
		VkRenderPassBeginInfo renderPassBeginInfo = 
		{
			/* sType           */ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			/* pNext           */ nullptr,
			/* renderPass      */ *m_renderPass,
			/* framebuffer     */ m_framebuffer->GetRendererFramebuffer(),
			/* renderArea      */ renderArea,
			/* clearValueCount */ clearValues.size(),
			/* pClearValues    */ clearValues.data()
		};
		
		m_chunkRenderList.Begin();
		
		{
			MCR_SCOPED_TIMER(0, "Render List Fill")
			
			if (m_enableOcclusionCulling)
			{
				m_visibilityCalculator.FillRenderList(m_chunkRenderList, *m_worldManager, camera, m_frustum);
				
				if (m_shouldCaptureVisibilityGraph)
				{
					m_visibilityGraph = std::make_unique<ChunkVisibilityGraph>(m_visibilityCalculator.GetVisibilityGraph(cb));
					m_shouldCaptureVisibilityGraph = false;
				}
			}
			else
			{
				m_worldManager->FillRenderList(m_chunkRenderList, m_frustum);
			}
		}
		
		{
			MCR_SCOPED_TIMER(0, "Render List End")
			m_chunkRenderList.End(cb);
		}
		
		{
			MCR_SCOPED_TIMER(0, "Render pass record")
			
			cb.BeginRenderPass(&renderPassBeginInfo);
			
			m_blockShader.Bind(cb, m_wireframe ? Shader::BindModes::Wireframe : Shader::BindModes::Default);
			
			cb.SetViewport(0, SingleElementSpan(viewport));
			cb.SetScissor(0, SingleElementSpan(renderArea));
			
			m_chunkRenderList.Render(cb);
			
			if (m_visibilityGraph)
			{
				m_debugShader.Bind(cb);
				m_debugShader.SetColor(cb, glm::vec4(0.3f, 1.0f, 0.3f, 0.3f));
				
				m_visibilityGraph->Draw(cb);
			}
			
			cb.EndRenderPass();
		}
		
		if (frameIndex % 16 == 0)
		{
			ChunkBufferAllocator::s_instance.ProcessFreedAllocations(cb);
		}
		
		cb.End();
	}
	
	void Renderer::FramebufferChanged(const Framebuffer& framebuffer)
	{
		m_framebuffer = &framebuffer;
		
		m_projectionMatrix = glm::perspectiveFov<float>(glm::half_pi<float>(), framebuffer.GetWidth(),
		                                                framebuffer.GetHeight(), 0.1f, 1000.0f);
		
		m_invProjectionMatrix = glm::inverse(m_projectionMatrix);
	}
}
