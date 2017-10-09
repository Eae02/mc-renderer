#include "cascadedshadowmapper.h"
#include "directionalshadowvolume.h"
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
			/* format         */ vulkan.depthFormat,
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
			/* dependencyCount */ static_cast<uint32_t>(subpassDependencies.size()),
			/* pDependencies   */ subpassDependencies.data()
		};
		
		VkRenderPass renderPass;
		CheckResult(vkCreateRenderPass(vulkan.device, &renderPassCreateInfo, nullptr, &renderPass));
		return renderPass;
	}
	
	static const VkDynamicState dynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	
	static const std::string_view setLayouts[] = { "BlockShaderShadow_Global" };
	
	static const Shader::CreateInfo shaderCreateInfo = 
	{
		/* vsName                  */ "block-shadow.vs",
		/* gsName                  */ "block-shadow.gs",
		/* fsName                  */ "block-shadow.fs",
		/* setLayoutNames          */ setLayouts,
		/* pushConstantRanges      */ { },
		/* vertexInputState        */ &blockVertexShadowInputState,
		/* topology                */ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		/* viewport                */ { 0, 0, 1, 1, 0, 1 },
		/* scissor                 */ { 0, 0, 1, 1 },
		/* enableDepthClamp        */ false,
		/* cullMode                */ VK_CULL_MODE_FRONT_BIT,
		/* frontFace               */ VK_FRONT_FACE_CLOCKWISE,
		/* enableDepthTest         */ true,
		/* enableDepthWrite        */ true,
		/* hasWireframeVariant     */ false,
		/* depthCompareOp          */ VK_COMPARE_OP_LESS,
		/* enableDepthBias         */ true,
		/* depthBiasConstantFactor */ 1.25f,
		/* depthBiasClamp          */ 0.0f,
		/* depthBiasSlopeFactor    */ 1.75f,
		/* attachmentBlendStates   */ { },
		/* dynamicState            */ dynamicState
	};
	
	CascadedShadowMapper::CascadedShadowMapper()
	    : m_renderPass(CreateRenderPass()), m_shader(*m_renderPass, shaderCreateInfo),
	      m_renderDescriptorSet("BlockShaderShadow_Global"), m_sampleDescriptorSet("ShadowSample")
	{
		SetResolution(2048);
		
		// ** Creates the light matrices host buffer **
		
		VkBufferCreateInfo hostBufferCreateInfo;
		InitBufferCreateInfo(hostBufferCreateInfo, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		                     sizeof(ShadowInfo) * SwapChain::GetImageCount());
		
		const VmaMemoryRequirements hostMemoryRequirements =
		{
			VMA_MEMORY_REQUIREMENT_PERSISTENT_MAP_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY
		};
		
		VmaAllocationInfo hostAllocationInfo;
		CheckResult(vmaCreateBuffer(vulkan.allocator, &hostBufferCreateInfo, &hostMemoryRequirements,
		                            m_infoHostBuffer.GetCreateAddress(), m_infoHostAllocation.GetCreateAddress(),
		                            &hostAllocationInfo));
		
		m_shadowInfoMemory = reinterpret_cast<ShadowInfo*>(hostAllocationInfo.pMappedData);
		
		// ** Creates the light matrices device buffer **
		
		VkBufferCreateInfo deviceBufferCreateInfo;
		InitBufferCreateInfo(deviceBufferCreateInfo, VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(ShadowInfo));
		
		const VmaMemoryRequirements deviceMemoryRequirements = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
		
		CheckResult(vmaCreateBuffer(vulkan.allocator, &deviceBufferCreateInfo, &deviceMemoryRequirements,
		                            m_infoDeviceBuffer.GetCreateAddress(), m_infoDeviceAllocation.GetCreateAddress(),
		                            nullptr));
		
		// ** Updates the shadow render descriptor set **
		
		VkWriteDescriptorSet descriptorWrites[4];
		
		const VkDescriptorBufferInfo infoBufferInfoR = { *m_infoDeviceBuffer, 0, sizeof(glm::mat4) * DirLightCascades };
		m_renderDescriptorSet.InitWriteDescriptorSet(descriptorWrites[0], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		                                             infoBufferInfoR);
		
		const VkDescriptorImageInfo blocksTextureInfo = BlocksTextureManager::GetInstance().GetImageInfo();
		m_renderDescriptorSet.InitWriteDescriptorSet(descriptorWrites[1], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		                                             blocksTextureInfo);
		
		// ** Updates the shadow sample descriptor set **
		
		const VkDescriptorImageInfo shadowMapInfo =
		{
			/* sampler     */ VK_NULL_HANDLE, //Immutable
			/* imageView   */ *m_shadowMapView,
			/* imageLayout */ VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
		};
		m_sampleDescriptorSet.InitWriteDescriptorSet(descriptorWrites[2], 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		                                             shadowMapInfo);
		
		const VkDescriptorBufferInfo infoBufferInfoS = { *m_infoDeviceBuffer, 0, sizeof(ShadowInfo) };
		m_sampleDescriptorSet.InitWriteDescriptorSet(descriptorWrites[3], 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		                                             infoBufferInfoS);
		
		UpdateDescriptorSets(descriptorWrites);
	}
	
	void CascadedShadowMapper::SetResolution(uint32_t resolution)
	{
		VkImageCreateInfo imageCreateInfo;
		InitImageCreateInfo(imageCreateInfo, VK_IMAGE_TYPE_2D, vulkan.depthFormat, resolution, resolution);
		imageCreateInfo.arrayLayers = DirLightCascades;
		imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		
		const VmaMemoryRequirements memoryRequirements = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
		
		CheckResult(vmaCreateImage(vulkan.allocator, &imageCreateInfo, &memoryRequirements,
		                           m_shadowMap.GetCreateAddress(), m_shadowMapAllocation.GetCreateAddress(), nullptr));
		
		VkImageViewCreateInfo imageViewCreateInfo;
		InitImageViewCreateInfo(imageViewCreateInfo, *m_shadowMap, VK_IMAGE_VIEW_TYPE_2D_ARRAY, vulkan.depthFormat,
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
		
		m_resolution = resolution;
	}
	
	void CascadedShadowMapper::CalcFrustumSlices(const ViewProjection& viewProj, float shadowEndDist)
	{
		float nextSliceBeginES = -ZNear;
		float nextSliceBeginPPS = 0;
		
		const float bias = 2;
		const float biasedN = bias * ZNear;
		const float offset = ZNear - biasedN;
		const float powBase = (shadowEndDist - offset) / biasedN;
		
		for (uint32_t i = 0; i < DirLightCascades; i++)
		{
			float mul = (i + 1) / static_cast<float>(DirLightCascades);
			float sliceEndES = -(biasedN * std::pow(powBase, mul) + offset);
			
			//Finds the slice end depth in post projection space
			glm::vec4 sliceEndVecPPS = viewProj.m_proj * glm::vec4(0, 0, sliceEndES, 1);
			float sliceEndPPS = sliceEndVecPPS.z / sliceEndVecPPS.w;
			
			m_frustumSlices[i].m_endDepth = sliceEndPPS;
			
			//The vertices which make up this slice, in post projection space
			glm::vec3 verticesPostProj[] =
			{
				{ -1, -1, nextSliceBeginPPS },
				{  1, -1, nextSliceBeginPPS },
				{ -1,  1, nextSliceBeginPPS },
				{  1,  1, nextSliceBeginPPS },
				
				{ -1, -1, sliceEndPPS },
				{  1, -1, sliceEndPPS },
				{ -1,  1, sliceEndPPS },
				{  1,  1, sliceEndPPS }
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
		float maxX = -std::numeric_limits<float>::infinity();
		float maxY = -std::numeric_limits<float>::infinity();
		float maxZ = -std::numeric_limits<float>::infinity();
		
		for (int i = 0; i < 8; i++)
		{
			glm::vec3 vertex(sliceMatrix * glm::vec4(slice.m_vertices[i], 1.0f));
			
			maxX = std::max(maxX, std::abs(vertex.x));
			maxY = std::max(maxY, std::abs(vertex.y));
			maxZ = std::max(maxZ, std::abs(vertex.z));
		}
		
		float volumeWidth = maxX * 2;
		float volumeHeight = maxY * 2;
		
		glm::vec2 volumeMin(-maxX, -maxY);
		glm::vec2 volumeMax(maxX, maxY);
		
		float texelSizeX = volumeWidth / static_cast<float>(m_resolution);
		float texelSizeY = volumeHeight / static_cast<float>(m_resolution);
		
		volumeMin.x = std::floor(volumeMin.x / texelSizeX) * texelSizeX;
		volumeMin.y = std::floor(volumeMin.y / texelSizeY) * texelSizeY;
		
		volumeMax.x = std::ceil(volumeMax.x / texelSizeX) * texelSizeX;
		volumeMax.y = std::ceil(volumeMax.y / texelSizeY) * texelSizeY;
		
		return glm::translate(glm::mat4(1), { -1, -1, 0 }) *
		       glm::scale(glm::mat4(1), { 2.0f / (volumeMax.x - volumeMin.x), 2.0f / (volumeMax.y - volumeMin.y), 0.5f / maxZ }) *
		       glm::translate(glm::mat4(1), { -volumeMin.x, -volumeMin.y, maxZ }) * 
		       glm::scale(glm::mat4(1), { 1, 1, 1 });
	}
	
	glm::mat4 CascadedShadowMapper::GetTexelAlignMatrix(const glm::mat4& lightMatrix)
	{
		glm::mat4 inverse = glm::inverse(lightMatrix);
		
		//Three of the vertices of the near plane of the light volume, in post projection space.
		glm::vec4 nearLightVolumeVerticesPPS[] =
		{
			{ -1, -1, 0, 1 },
			{  1, -1, 0, 1 },
			{ -1,  1, 0, 1 }
		};
		
		//Transforms the near plane vertices to world space.
		glm::vec3 nearLightVolumeVerticesWS[3];
		for (int i = 0; i < 3; i++)
		{
			glm::vec4 vertexWS4 = inverse * nearLightVolumeVerticesPPS[i];
			nearLightVolumeVerticesWS[i] = glm::vec3(vertexWS4) / vertexWS4.w;
		}
		
		glm::vec3 a = nearLightVolumeVerticesWS[1] - nearLightVolumeVerticesWS[0];
		glm::vec3 b = nearLightVolumeVerticesWS[2] - nearLightVolumeVerticesWS[0];
		
		float volumeWidthWS = glm::length(a);
		float volumeHeightWS = glm::length(b);
		
		a /= volumeWidthWS;
		b /= volumeHeightWS;
		
		//The dimensions of a shadowmap texel, in world space
		float worldTexelSizeX = volumeWidthWS / static_cast<float>(m_resolution);
		float worldTexelSizeY = volumeHeightWS / static_cast<float>(m_resolution);
		
		float volumeX = glm::dot(a, nearLightVolumeVerticesWS[0]);
		float volumeY = glm::dot(b, nearLightVolumeVerticesWS[0]);
		
		float nextMultipleX = std::floor(volumeX / worldTexelSizeX) * worldTexelSizeX;
		float nextMultipleY = std::floor(volumeY / worldTexelSizeY) * worldTexelSizeY;
		
		return glm::translate(glm::mat4(1), a * (volumeX - nextMultipleX) + b * (volumeY - nextMultipleY));
	}
	
	void CascadedShadowMapper::CalculateSlices(const glm::vec3& lightDirection, const ViewProjection& viewProjection,
	                                           const WorldManager& worldManager)
	{
		const float shadowEndDist = 100;
		
		const glm::vec3 lightDirectionL = glm::cross(lightDirection, worldManager.GetCamera().GetForward());
		const glm::mat3 rotationMatrix(lightDirectionL, glm::cross(lightDirectionL, lightDirection), lightDirection);
		const glm::mat4 rotationMatrixInv = glm::transpose(rotationMatrix);
		
		CalcFrustumSlices(viewProjection, shadowEndDist);
		
		std::array<glm::mat4, DirLightCascades> invLightMatrices;
		
		glm::mat4* lightMatrices = m_shadowInfoMemory[frameQueueIndex].m_lightMatrices;
		
		for (uint32_t i = 0; i < DirLightCascades; i++)
		{
			//Calculates the light-space matrix for this cascade
			glm::mat4 sliceMatrix = rotationMatrixInv * glm::translate(glm::mat4(1.0f), -m_frustumSlices[i].m_center);
			lightMatrices[i] = GetCascadeProjectionMatrix(sliceMatrix, m_frustumSlices[i]) * sliceMatrix;
			lightMatrices[i] = lightMatrices[i] * GetTexelAlignMatrix(lightMatrices[i]);
			
			m_shadowInfoMemory[frameQueueIndex].m_sliceEndDepths[i] = glm::vec4(m_frustumSlices[i].m_endDepth);
			
			invLightMatrices[i] = glm::inverse(lightMatrices[i]);
		}
		
		m_shadowVolume = DirectionalShadowVolume(invLightMatrices.data(), lightDirection);
	}
	
	void CascadedShadowMapper::Render(CommandBuffer& commandBuffer, const ChunkRenderList& shadowRenderList)
	{
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
}