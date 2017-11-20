#include "renderer.h"
#include "frustum.h"
#include "framebuffer.h"
#include "constants.h"
#include "regions/chunkbufferallocator.h"
#include "../loadcontext.h"
#include "../profiling/profiling.h"
#include "../world/worldmanager.h"
#include "../profiling/frameprofiler.h"
#include "../timemanager.h"
#include "fbformats.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

namespace MCR
{
	VkRenderPass Renderer::CreateRenderPass()
	{
		std::array<VkAttachmentDescription, 2> attachments = { };
		attachments[0].format = FBFormats::HDRColor;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; //Image will be written to by the sky renderer
		
		attachments[1].format = vulkan.depthStencilFormat;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; //Image will be sampled by the sky shader
		
		VkAttachmentReference colorAttachmentRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkAttachmentReference depthStencilAttachmentRef = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
		
		std::array<VkSubpassDescription, 1> subpassDescriptions = { };
		subpassDescriptions[0].colorAttachmentCount = 1;
		subpassDescriptions[0].pColorAttachments = &colorAttachmentRef;
		subpassDescriptions[0].pDepthStencilAttachment = &depthStencilAttachmentRef;
		
		std::array<VkSubpassDependency, 1> dependencies;
		dependencies[0].srcSubpass = 0;
		dependencies[0].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dependencyFlags = 0;
		
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
	
	VkRenderPass Renderer::CreateWaterRenderPass()
	{
		std::array<VkAttachmentDescription, 2> attachments = { };
		attachments[0].format = FBFormats::HDRColor;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; //Image has not been used yet
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; //Image will be read by the post processor
		
		attachments[1].format = vulkan.depthStencilFormat;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; //Image has not been used yet
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
	    : m_renderPass(CreateRenderPass()), m_waterRenderPass(CreateWaterRenderPass()),
	      m_shadowMapper(m_renderSettingsBuffer.GetBufferInfo()),
	      m_blockShader({ *m_renderPass, 0 }, m_renderSettingsBuffer.GetBufferInfo()),
	      m_waterShader({ *m_renderPass, 0 }, m_renderSettingsBuffer.GetBufferInfo()),
	      m_waterPostShader({ *m_renderPass, 0 }, m_renderSettingsBuffer.GetBufferInfo()),
	      m_debugShader({ *m_renderPass, 0 }, m_renderSettingsBuffer.GetBufferInfo()),
	      m_skyRenderer(m_renderSettingsBuffer.GetBufferInfo())
	{
		m_commandBuffers.reserve(SwapChain::GetImageCount());
		for (uint32_t i = 0; i < SwapChain::GetImageCount(); i++)
		{
			m_commandBuffers.emplace_back(vulkan.stdCommandPools[QUEUE_FAMILY_GRAPHICS]);
		}
		
		m_waterShader.SetCausticsTexture(m_causticsTexture);
		m_waterPostShader.SetCausticsTexture(m_causticsTexture);
	}
	
	void Renderer::Render(const RenderParams& params)
	{
		CommandBuffer& cb = m_commandBuffers[frameQueueIndex];
		
		cb.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		
		const Camera& camera = m_worldManager->GetCamera();
		
		ViewProjection viewProj;
		viewProj.m_proj = m_projectionMatrix;
		viewProj.m_invProj = m_invProjectionMatrix;
		viewProj.m_view = camera.GetViewMatrix();
		viewProj.m_invView = camera.GetInverseViewMatrix();
		viewProj.m_viewProj = viewProj.m_proj * viewProj.m_view;
		viewProj.m_invViewProj = viewProj.m_invView * viewProj.m_invProj;
		
#ifdef MCR_DEBUG
		currentFrameProfiler->Reset(cb);
#endif
		
		m_renderSettingsBuffer.SetData(cb, viewProj, camera.GetPosition(), params.m_time, *params.m_timeManager);
		
		{
			MCR_SCOPED_TIMER(0, "Water Build");
			m_worldManager->BuildWater(cb);
		}
		
		if (!m_isFrustumFrozen)
		{
			m_frustum = Frustum(viewProj.m_invViewProj);
		}
		
		m_shadowMapper.CalculateSlices(params.m_timeManager->GetShadowDirection(), viewProj, *m_worldManager);
		
		if (m_shouldCaptureShadowVolume)
		{
			m_shadowVolume = std::make_unique<ShadowVolumeMesh>(m_shadowMapper.GetShadowVolume(), cb);
			m_frustumVolume = std::make_unique<ShadowVolumeMesh>(m_shadowMapper.GetSliceMesh(cb));
			m_shouldCaptureShadowVolume = false;
		}
		
		m_chunkRenderList.Begin();
		m_shadowRenderList.Begin();
		
		{
			MCR_SCOPED_TIMER(0, "Render List Fill");
			
			if (m_enableOcclusionCulling)
			{
				m_visibilityCalculator.FillRenderList(m_chunkRenderList, m_shadowRenderList, *m_worldManager, camera,
				                                      m_frustum, m_shadowMapper.GetShadowVolume());
				
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
			MCR_SCOPED_TIMER(0, "Render List End");
			m_chunkRenderList.End(cb);
			m_shadowRenderList.End(cb);
		}
		
		{
			MCR_SCOPED_TIMER(0, "Shadow record");
			
			uint32_t gpuTimer = BeginGPUTimer(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, "Shadow Render");
			
			m_shadowMapper.Render(cb, m_shadowRenderList);
			
			EndGPUTimer(cb, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, gpuTimer);
		}
		
		{
			MCR_SCOPED_TIMER(0, "Render pass record");
			
			VkRect2D renderArea;
			VkViewport viewport;
			m_framebuffer->GetViewportAndRenderArea(renderArea, viewport);
			
			// ** Main rendering **
			
			std::array<VkClearValue, 2> clearValues;
			SetColorClearValue(clearValues[0], glm::vec4 { 0.0f, 0.0f, 0.0f, 1.0f });
			SetDepthStencilClearValue(clearValues[1], 1.0f, 0);
			
			const VkRenderPassBeginInfo renderPassBeginInfo = 
			{
				/* sType           */ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				/* pNext           */ nullptr,
				/* renderPass      */ *m_renderPass,
				/* framebuffer     */ m_framebuffer->GetRendererFramebuffer(),
				/* renderArea      */ renderArea,
				/* clearValueCount */ clearValues.size(),
				/* pClearValues    */ clearValues.data()
			};
			
			cb.BeginRenderPass(&renderPassBeginInfo);
			
			uint32_t mainRenderGPUTimer = BeginGPUTimer(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, "Main Render");
			
			m_blockShader.Bind(cb, m_shadowMapper.GetDescriptorSet(),
			                   m_wireframe ? Shader::BindModes::Wireframe : Shader::BindModes::Default);
			
			cb.SetViewport(0, SingleElementSpan(viewport));
			cb.SetScissor(0, SingleElementSpan(renderArea));
			
			m_chunkRenderList.Render(cb);
			
			if (m_visibilityGraph)
			{
				m_debugShader.Bind(cb, Shader::BindModes::Default);
				m_debugShader.SetColor(cb, glm::vec4(0.3f, 1.0f, 0.3f, 0.3f));
				
				m_visibilityGraph->Draw(cb);
			}
			
			if (m_shadowVolume)
			{
				m_shadowVolume->Draw(cb, m_debugShader);
			}
			
			if (m_frustumVolume)
			{
				m_frustumVolume->Draw(cb, m_debugShader);
			}
			
			cb.EndRenderPass();
			
			EndGPUTimer(cb, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, mainRenderGPUTimer);
			
			// ** Sky rendering **
			
			m_skyRenderer.Render(cb, *m_framebuffer);
			
			// ** Water rendering **
			
			uint32_t waterRenderGPUTimer = BeginGPUTimer(cb, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, "Water Render");
			
			VkClearValue waterClearValues[2] = { };
			waterClearValues[1].depthStencil.stencil = 0;
			const VkRenderPassBeginInfo waterRenderPassBeginInfo =
			{
				/* sType           */ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				/* pNext           */ nullptr,
				/* renderPass      */ *m_waterRenderPass,
				/* framebuffer     */ m_framebuffer->GetWaterFramebuffer(),
				/* renderArea      */ renderArea,
				/* clearValueCount */ static_cast<uint32_t>(ArrayLength(waterClearValues)),
				/* pClearValues    */ waterClearValues
			};
			
			cb.BeginRenderPass(&waterRenderPassBeginInfo);
			
			float waterPlaneY = 0;
			bool underwater = m_worldManager->IsCameraUnderWater(waterPlaneY);
			
			m_waterShader.Bind(cb, m_shadowMapper.GetDescriptorSet(), underwater);
			
			m_chunkRenderList.RenderWater(cb);
			
			m_waterPostShader.Bind(cb, m_shadowMapper.GetDescriptorSet(), underwater, waterPlaneY);
			
			cb.Draw(3, 1, 0, 0);
			
			cb.EndRenderPass();
			
			EndGPUTimer(cb, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, waterRenderGPUTimer);
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
		
		m_projectionMatrix = glm::perspectiveFov<float>(glm::radians(FieldOfView), framebuffer.GetWidth(),
		                                                framebuffer.GetHeight(), ZNear, ZFar);
		
		m_invProjectionMatrix = glm::inverse(m_projectionMatrix);
		
		m_skyRenderer.FramebufferChanged(framebuffer);
		m_waterShader.FramebufferChanged(framebuffer);
		m_waterPostShader.FramebufferChanged(framebuffer);
	}
	
	static constexpr bool enableCausticsCache = true;
	
	static fs::path GetCausticsCachePath()
	{
		return GetAppDataPath() / "caustics_cache";
	}
	
	void Renderer::Initialize(LoadContext& loadContext)
	{
		if (!enableCausticsCache || !m_causticsTexture.TryLoad(GetCausticsCachePath(), loadContext))
		{
			m_causticsTexture.Render(loadContext.GetCB());
		}
		
		m_waterShader.Initialize(loadContext);
	}
	
	void Renderer::SaveCausticsTexture() const
	{
		const fs::path causticsCachePath = GetCausticsCachePath();
		
		if (enableCausticsCache && !fs::exists(causticsCachePath))
		{
			m_causticsTexture.Save(causticsCachePath);
		}
	}
}
