#include "cascadedshadowmapper.h"
#include "directionalshadowvolume.h"
#include "../windnoiseimage.h"
#include "../frustum.h"
#include "../constants.h"
#include "../vertex.h"
#include "../../world/worldmanager.h"
#include "../../world/camera.h"
#include "../../blocks/blockstexturemanager.h"

#include <glm/gtc/matrix_transform.hpp>

namespace MCR
{
	static VkRenderPass CreateRenderPass()
	{
		const VkAttachmentDescription attachmentDescription = 
		{
			/* flags          */ 0,
			/* format         */ vulkan.depthStencilFormat,
			/* samples        */ VK_SAMPLE_COUNT_1_BIT,
			/* loadOp         */ VK_ATTACHMENT_LOAD_OP_CLEAR,
			/* storeOp        */ VK_ATTACHMENT_STORE_OP_STORE,
			/* stencilLoadOp  */ VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			/* stencilStoreOp */ VK_ATTACHMENT_STORE_OP_DONT_CARE,
			/* initialLayout  */ VK_IMAGE_LAYOUT_UNDEFINED,
			/* finalLayout    */ VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
		};
		
		const VkAttachmentReference attachmentRef = { 0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
		
		VkSubpassDescription subpassDescription = { };
		subpassDescription.pDepthStencilAttachment = &attachmentRef;
		
		std::array<VkSubpassDependency, 2> subpassDependencies;
		
		subpassDependencies[0] = 
		{
			/* srcSubpass      */ VK_SUBPASS_EXTERNAL,
			/* dstSubpass      */ 0,
			/* srcStageMask    */ VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			/* dstStageMask    */ VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			/* srcAccessMask   */ VK_ACCESS_SHADER_READ_BIT,
			/* dstAccessMask   */ VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			/* dependencyFlags */ VK_DEPENDENCY_BY_REGION_BIT
		};
		
		subpassDependencies[1] = 
		{
			/* srcSubpass      */ 0,
			/* dstSubpass      */ VK_SUBPASS_EXTERNAL,
			/* srcStageMask    */ VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			/* dstStageMask    */ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			/* srcAccessMask   */ VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			/* dstAccessMask   */ VK_ACCESS_SHADER_READ_BIT,
			/* dependencyFlags */ VK_DEPENDENCY_BY_REGION_BIT
		};
		
		const VkRenderPassCreateInfo renderPassCreateInfo = 
		{
			/* sType           */ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			/* pNext           */ nullptr,
			/* flags           */ 0,
			/* attachmentCount */ 1,
			/* pAttachments    */ &attachmentDescription,
			/* subpassCount    */ 1,
			/* pSubpasses      */ &subpassDescription,
			/* dependencyCount */ static_cast<uint32_t>(subpassDependencies.size() - 1),
			/* pDependencies   */ subpassDependencies.data() + 1
		};
		
		VkRenderPass renderPass;
		CheckResult(vkCreateRenderPass(vulkan.device, &renderPassCreateInfo, nullptr, &renderPass));
		return renderPass;
	}
	
	static const VkDynamicState dynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	
	static const std::string_view setLayouts[] = { "BlockShaderShadow_Global" };
	
	static const Shader::CreateInfo shaderCreateInfo = Shader::CreateInfo()
		.SetVertexShaderName("block-shadow.vs")
		.SetGeometryShaderName("block-shadow.gs")
		.SetFragmentShaderName("block-shadow.fs")
		.SetDSLayoutNames(setLayouts)
		.SetVertexInputState(&blockVertexShadowInputState)
		.SetCullMode(VK_CULL_MODE_FRONT_BIT)
		.SetEnableDepthTest(true)
		.SetEnableDepthWrite(true)
		.SetEnableDepthBias(true)
		.SetDepthBiasConstantFactor(2.25f)
		.SetDepthBiasSlopeFactor(1.75f)
		.SetDepthBiasClamp(0.0f)
		.SetDynamicState(dynamicState);
	
	CascadedShadowMapper::CascadedShadowMapper(const VkDescriptorBufferInfo& renderSettingsBufferInfo)
	    : m_renderPass(CreateRenderPass()), m_shader({ *m_renderPass, 0 }, shaderCreateInfo),
	      m_renderDescriptorSet("BlockShaderShadow_Global"), m_sampleDescriptorSet("ShadowSample")
	{
		SetQualityLevel(QualityLevels::Medium);
		
		// ** Creates the light matrices host buffer **
		
		VkBufferCreateInfo hostBufferCreateInfo;
		InitBufferCreateInfo(hostBufferCreateInfo, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		                     sizeof(ShadowInfo) * SwapChain::GetImageCount());
		
		const VmaAllocationCreateInfo hostAllocationCI =
		{
			VMA_ALLOCATION_CREATE_MAPPED_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY
		};
		
		VmaAllocationInfo hostAllocationInfo;
		CheckResult(vmaCreateBuffer(vulkan.allocator, &hostBufferCreateInfo, &hostAllocationCI,
		                            m_infoHostBuffer.GetCreateAddress(), m_infoHostAllocation.GetCreateAddress(),
		                            &hostAllocationInfo));
		
		m_shadowInfoMemory = reinterpret_cast<ShadowInfo*>(hostAllocationInfo.pMappedData);
		
		// ** Creates the light matrices device buffer **
		
		VkBufferCreateInfo deviceBufferCreateInfo;
		InitBufferCreateInfo(deviceBufferCreateInfo, VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(ShadowInfo));
		
		const VmaAllocationCreateInfo deviceAllocationCI = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
		
		CheckResult(vmaCreateBuffer(vulkan.allocator, &deviceBufferCreateInfo, &deviceAllocationCI,
		                            m_infoDeviceBuffer.GetCreateAddress(), m_infoDeviceAllocation.GetCreateAddress(),
		                            nullptr));
		
		// ** Updates the shadow render descriptor set **
		
		VkWriteDescriptorSet descriptorWrites[5];
		
		//Render settings buffer
		m_renderDescriptorSet.InitWriteDescriptorSet(descriptorWrites[0], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		                                             renderSettingsBufferInfo);
		
		//Light matrices buffer
		const VkDescriptorBufferInfo infoBufferInfoR = { *m_infoDeviceBuffer, 0, sizeof(glm::mat4) * DirLightCascades };
		m_renderDescriptorSet.InitWriteDescriptorSet(descriptorWrites[1], 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		                                             infoBufferInfoR);
		
		//Blocks texture array
		const VkDescriptorImageInfo blocksTextureInfo = BlocksTextureManager::GetInstance().GetImageInfo();
		m_renderDescriptorSet.InitWriteDescriptorSet(descriptorWrites[2], 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		                                             blocksTextureInfo);
		
		//Wind noise texture
		const VkDescriptorImageInfo windNoiseTextureInfo = WindNoiseImage::GetInstance().GetImageInfo();
		m_renderDescriptorSet.InitWriteDescriptorSet(descriptorWrites[3], 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		                                             windNoiseTextureInfo);
		
		// ** Updates the shadow sample descriptor set **
		
		const VkDescriptorBufferInfo infoBufferInfoS = { *m_infoDeviceBuffer, 0, sizeof(ShadowInfo) };
		m_sampleDescriptorSet.InitWriteDescriptorSet(descriptorWrites[4], 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		                                             infoBufferInfoS);
		
		UpdateDescriptorSets(descriptorWrites);
	}
	
	void CascadedShadowMapper::SetQualityLevel(QualityLevels qualityLevel)
	{
		const uint32_t resolutions[] = { 512, 1024, 2048, 4096 };
		const float endDistances[] = { 60, 70, 80, 100 };
		
		if (m_resolution != resolutions[static_cast<int>(qualityLevel)])
		{
			m_resolutionChanged = true;
		}
		
		m_resolution = resolutions[static_cast<int>(qualityLevel)];
		m_endDistance = endDistances[static_cast<int>(qualityLevel)];
	}
	
	void CascadedShadowMapper::CreateFramebuffer(uint32_t resolution)
	{
		VkImageCreateInfo imageCreateInfo;
		InitImageCreateInfo(imageCreateInfo, VK_IMAGE_TYPE_2D, vulkan.depthStencilFormat, resolution, resolution);
		imageCreateInfo.arrayLayers = DirLightCascades;
		imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		
		const VmaAllocationCreateInfo allocationCI = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
		
		CheckResult(vmaCreateImage(vulkan.allocator, &imageCreateInfo, &allocationCI, m_shadowMap.GetCreateAddress(),
		                           m_shadowMapAllocation.GetCreateAddress(), nullptr));
		
		VkImageViewCreateInfo imageViewCreateInfo;
		InitImageViewCreateInfo(imageViewCreateInfo, *m_shadowMap, VK_IMAGE_VIEW_TYPE_2D_ARRAY, vulkan.depthStencilFormat,
		                        { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, DirLightCascades });
		
		CheckResult(vkCreateImageView(vulkan.device, &imageViewCreateInfo, nullptr,
		                              m_shadowMapView.GetCreateAddress()));
		
		const VkFramebufferCreateInfo framebufferCreateInfo = 
		{
			/* sType           */ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			/* pNext           */ nullptr,
			/* flags           */ 0,
			/* renderPass      */ *m_renderPass,
			/* attachmentCount */ 1,
			/* pAttachments    */ &*m_shadowMapView,
			/* width           */ resolution,
			/* height          */ resolution,
			/* layers          */ DirLightCascades
		};
		
		CheckResult(vkCreateFramebuffer(vulkan.device, &framebufferCreateInfo, nullptr,
		                                m_framebuffer.GetCreateAddress()));
		
		//Updates the sampler descriptor set with the new image view.
		const VkDescriptorImageInfo shadowMapInfo =
			{
				/* sampler     */ VK_NULL_HANDLE, //Immutable
				/* imageView   */ *m_shadowMapView,
				/* imageLayout */ VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
			};
		VkWriteDescriptorSet descriptorWrite;
		m_sampleDescriptorSet.InitWriteDescriptorSet(descriptorWrite, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		                                             shadowMapInfo);
		UpdateDescriptorSets(SingleElementSpan(descriptorWrite));
	}
	
	void CascadedShadowMapper::CalcFrustumSlices(const ViewProjection& viewProj, float shadowEndDist)
	{
		float nextSliceBeginES = -ZNear;
		float nextSliceBeginPPS = 0;
		
		const float bias = 25;
		const float biasedN = bias * ZNear;
		const float offset = ZNear - biasedN;
		const float powBase = (shadowEndDist - offset) / biasedN;
		const float linBias = 0.0f;
		
		for (uint32_t i = 0; i < DirLightCascades; i++)
		{
			float mul = (i + 1) / static_cast<float>(DirLightCascades);
			float sliceEndESExp = -(biasedN * std::pow(powBase, mul) + offset);
			float sliceEndESLin = -((shadowEndDist - offset) * mul + offset);
			float sliceEndES = glm::mix(sliceEndESExp, sliceEndESLin, linBias);
			
			//Finds the slice end depth in post projection space
			glm::vec4 sliceEndVecPPS = viewProj.m_proj * glm::vec4(0, 0, sliceEndES, 1);
			float sliceEndPPS = sliceEndVecPPS.z / sliceEndVecPPS.w;
			
			m_frustumSlices[i].m_endDepth = sliceEndPPS;
			
			//The vertices which make up this slice, in post projection space
			glm::vec3 verticesPostProj[] =
			{
				{ -1,  1, nextSliceBeginPPS },
				{  1,  1, nextSliceBeginPPS },
				{  1, -1, nextSliceBeginPPS },
				{ -1, -1, nextSliceBeginPPS },
				
				{ -1,  1, sliceEndPPS },
				{  1,  1, sliceEndPPS },
				{  1, -1, sliceEndPPS },
				{ -1, -1, sliceEndPPS }
			};
			
			//Transforms the frustum's vertices into world space
			for (int v = 0; v < 8; v++)
			{
				glm::vec4 vertexWS = viewProj.m_invViewProj * glm::vec4(verticesPostProj[v], 1.0f);
				m_frustumSlices[i].m_vertices[v] = glm::vec3(vertexWS) / vertexWS.w;
			}
			
			glm::vec4 centerES(0, 0, (nextSliceBeginES + sliceEndES) / 2, 1.0f);
			m_frustumSlices[i].m_center = glm::vec3(viewProj.m_invView * centerES);
			
			nextSliceBeginES = sliceEndES;
			nextSliceBeginPPS = sliceEndPPS;
		}
	}
	
	//Calculates the projection matrix for a cascade, provided it's "view" matrix and frustum slice
	glm::mat4 CascadedShadowMapper::GetCascadeProjectionMatrix(const glm::mat4& sliceMatrix,
	                                                           const CascadedShadowMapper::FrustumSlice& slice)
	{
		float minX = std::numeric_limits<float>::infinity();
		float minY = std::numeric_limits<float>::infinity();
		float minZ = std::numeric_limits<float>::infinity();
		
		float maxX = -std::numeric_limits<float>::infinity();
		float maxY = -std::numeric_limits<float>::infinity();
		float maxZ = -std::numeric_limits<float>::infinity();
		
		for (int i = 0; i < 8; i++)
		{
			glm::vec3 vertex(sliceMatrix * glm::vec4(slice.m_vertices[i], 1.0f));
			
			minX = std::min(minX, vertex.x);
			minY = std::min(minY, vertex.y);
			minZ = std::min(minZ, vertex.z);
			
			maxX = std::max(maxX, vertex.x);
			maxY = std::max(maxY, vertex.y);
			maxZ = std::max(maxZ, vertex.z);
		}
		
		return glm::translate(glm::mat4(1), { -1, -1, 0 }) *
		       glm::scale(glm::mat4(1), { 2.0f / (maxX - minX), 2.0f / (maxY - minY), 1.0f / (maxZ - minZ) }) *
		       glm::translate(glm::mat4(1), { -minX, -minY, -minZ });
	}
	
	void CascadedShadowMapper::CalculateSlices(const glm::vec3& lightDirection, const ViewProjection& viewProjection,
	                                           const WorldManager& worldManager)
	{
		const glm::vec3 lightDirLeft = glm::cross(worldManager.GetCamera().GetForward(), lightDirection);
		const glm::mat3 rotationMatrix(lightDirLeft, glm::cross(lightDirLeft, lightDirection), lightDirection);
		const glm::mat4 rotationMatrixInv(glm::transpose(rotationMatrix));
		
		CalcFrustumSlices(viewProjection, m_endDistance);
		
		std::array<glm::mat4, DirLightCascades> invLightMatrices;
		
		glm::mat4* lightMatrices = m_shadowInfoMemory[frameQueueIndex].m_lightMatrices;
		
		for (uint32_t i = 0; i < DirLightCascades; i++)
		{
			//Calculates the light-space matrix for this cascade
			lightMatrices[i] = GetCascadeProjectionMatrix(rotationMatrixInv, m_frustumSlices[i]) * rotationMatrixInv;
			
			m_shadowInfoMemory[frameQueueIndex].m_sliceEndDepths[i] = glm::vec4(m_frustumSlices[i].m_endDepth);
			
			invLightMatrices[i] = glm::inverse(lightMatrices[i]);
		}
		
		m_shadowVolume = DirectionalShadowVolume(invLightMatrices.data(), lightDirection);
	}
	
	void CascadedShadowMapper::Render(CommandBuffer& commandBuffer, const ChunkRenderList& shadowRenderList)
	{
		if (m_resolutionChanged)
		{
			CreateFramebuffer(m_resolution);
			m_resolutionChanged = false;
		}
		
		const VkRect2D renderArea = { { }, { m_resolution, m_resolution } };
		
		VkClearValue clearValue;
		clearValue.depthStencil = { 1.0f, 0 };
		
		//Uploads the shadow info buffer
		commandBuffer.CopyBuffer(*m_infoHostBuffer, *m_infoDeviceBuffer,
		                         { sizeof(ShadowInfo) * frameQueueIndex, 0, sizeof(ShadowInfo) });
		
		//Inserts a barrier for the shadow info buffer
		const VkBufferMemoryBarrier bufferBarrier = 
		{
			/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			/* pNext               */ nullptr,
			/* srcAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
			/* dstAccessMask       */ VK_ACCESS_UNIFORM_READ_BIT,
			/* srcQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
			/* dstQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
			/* buffer              */ *m_infoDeviceBuffer,
			/* offset              */ 0,
			/* size                */ VK_WHOLE_SIZE
		};
		commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, 0,
		                              { }, SingleElementSpan(bufferBarrier), { });
		
		VkRenderPassBeginInfo renderPassBeginInfo = 
		{
			/* sType           */ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			/* pNext           */ nullptr,
			/* renderPass      */ *m_renderPass,
			/* framebuffer     */ *m_framebuffer,
			/* renderArea      */ renderArea,
			/* clearValueCount */ 1,
			/* pClearValues    */ &clearValue
		};
		commandBuffer.BeginRenderPass(&renderPassBeginInfo);
		
		m_shader.Bind(commandBuffer);
		
		const VkViewport viewport =
		{
			0.0f, 0.0f, static_cast<float>(m_resolution), static_cast<float>(m_resolution), 0.0f, 1.0f
		};
		
		commandBuffer.SetViewport(0, SingleElementSpan(viewport));
		commandBuffer.SetScissor(0, SingleElementSpan(renderArea));
		
		const VkDescriptorSet descriptorSets[] = { *m_renderDescriptorSet };
		commandBuffer.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_shader.GetLayout(), 0, descriptorSets, { });
		
		shadowRenderList.Render(commandBuffer);
		
		commandBuffer.EndRenderPass();
	}
	
	ShadowVolumeMesh CascadedShadowMapper::GetSliceMesh(CommandBuffer& cb)
	{
		std::array<glm::vec3, DirLightCascades * 8> positions;
		
		glm::vec3* positionsOut = positions.data();
		for (const FrustumSlice& slice : m_frustumSlices)
		{
			for (const glm::vec3& vPosition : slice.m_vertices)
			{
				*(positionsOut++) = vPosition;
			}
		}
		
		return ShadowVolumeMesh(positions.data(), cb);
	}
}
