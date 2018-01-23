#include "starrenderer.h"
#include "../loadcontext.h"
#include "blendstates.h"
#include "fbformats.h"
#include "framebuffer.h"
#include "../timemanager.h"

#include <random>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/color_space.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace MCR
{
	const VkVertexInputBindingDescription StarRenderer::s_vertexInputBindings[] =
	{
		{
			/* binding   */ 0,
			/* stride    */ sizeof(Star),
			/* inputRate */ VK_VERTEX_INPUT_RATE_INSTANCE
		}
	};
	
	const VkVertexInputAttributeDescription StarRenderer::s_vertexInputAttributes[] =
	{
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Star, m_color) },
		{ 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Star, m_direction) }
	};
	
	const VkPipelineVertexInputStateCreateInfo StarRenderer::s_vertexInputState = 
	{
		/* sType                           */ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		/* pNext                           */ nullptr,
		/* flags                           */ 0,
		/* vertexBindingDescriptionCount   */ ArrayLength(s_vertexInputBindings),
		/* pVertexBindingDescriptions      */ s_vertexInputBindings,
		/* vertexAttributeDescriptionCount */ ArrayLength(s_vertexInputAttributes),
		/* pVertexAttributeDescriptions    */ s_vertexInputAttributes
	};
	
	static const VkDynamicState dynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	
	static const std::string_view setLayouts[] = { "Star" };
	
	const VkPushConstantRange pushConstantRange = { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * (4 * 4) };
	
	const Shader::CreateInfo StarRenderer::s_starShaderCreateInfo = Shader::CreateInfo()
		.SetVertexShaderName("star.vs")
		.SetFragmentShaderName("star.fs")
		.SetDSLayoutNames(setLayouts)
		.SetPushConstantRanges(SingleElementSpan((pushConstantRange)))
		.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.SetVertexInputState(&s_vertexInputState)
		.SetEnableDepthClamp(false)
		.SetCullMode(VK_CULL_MODE_NONE)
		.SetEnableDepthTest(false)
		.SetEnableDepthWrite(false)
		.SetHasWireframeVariant(true)
		.SetAttachmentBlendStates(SingleElementSpan(BlendStates::additive))
		.SetDynamicState(dynamicState);
	
	StarRenderer::StarRenderer(Shader::RenderPassInfo renderPassInfo,
	                           const VkDescriptorBufferInfo& renderSettingsBufferInfo)
		: m_starShader(renderPassInfo, s_starShaderCreateInfo), m_descriptorSet("Star")
	{
		VkWriteDescriptorSet renderSettingsDSWrite;
		m_descriptorSet.InitWriteDescriptorSet(renderSettingsDSWrite, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		                                       renderSettingsBufferInfo);
		UpdateDescriptorSets(SingleElementSpan(renderSettingsDSWrite));
	}
	
	static const size_t numStars = 5000;
	
	void StarRenderer::Initialize(LoadContext& loadContext)
	{
		const uint64_t starBufferSize = numStars * sizeof(Star);
		
		VkBuffer stagingBuffer;
		VmaAllocation stagingBufferAllocation;
		void* stagingBufferMemory;
		CreateStagingBuffer(starBufferSize, &stagingBuffer, &stagingBufferAllocation, &stagingBufferMemory);
		
		Star* stars = reinterpret_cast<Star*>(stagingBufferMemory);
		
		std::mt19937 rand;
		std::uniform_real_distribution<float> thetaDist(0, glm::two_pi<float>());
		std::uniform_real_distribution<float> pitchDist(-1.0f, 1.0f);
		
		std::uniform_real_distribution<float> intensityDist(0.1f, 10.0f);
		std::uniform_real_distribution<float> distanceDist(300000, 1000000);
		
		glm::vec3 colorB = glm::convertSRGBToLinear(glm::vec3(0.92f, 0.98f, 1.0f));
		glm::vec3 colorR = glm::convertSRGBToLinear(glm::vec3(1.0f, 0.97f, 0.97f));
		std::uniform_real_distribution<float> colorFadeDist(0.0f, 1.0f);
		const float colorPow = 3.0f;
		
		for (size_t i = 0; i < numStars; i++)
		{
			const float theta = thetaDist(rand);
			const float pitch = std::acos(pitchDist(rand));
			const float sinPitch = std::sin(pitch);
			
			stars[i].m_color = glm::pow(glm::mix(colorR, colorB, colorFadeDist(rand)), glm::vec3(colorPow)) * intensityDist(rand);
			stars[i].m_distance = distanceDist(rand);
			stars[i].m_direction = glm::vec3(std::cos(theta) * sinPitch, std::sin(theta) * sinPitch, std::cos(pitch));
		}
		
		VkBufferCreateInfo bufferCreateInfo;
		InitBufferCreateInfo(bufferCreateInfo, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		                     starBufferSize);
		
		VmaAllocationCreateInfo allocationCreateInfo = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
		
		CheckResult(vmaCreateBuffer(vulkan.allocator, &bufferCreateInfo, &allocationCreateInfo,
		                            m_starFieldBuffer.GetCreateAddress(), m_starFieldAllocation.GetCreateAddress(),
		                            nullptr));
		
		loadContext.GetCB().CopyBuffer(stagingBuffer, *m_starFieldBuffer, { 0, 0, starBufferSize });
		loadContext.TakeResource(VkHandle<VkBuffer>(stagingBuffer));
		loadContext.TakeResource(VkHandle<VmaAllocation>(stagingBufferAllocation));
	}
	
	void StarRenderer::FramebufferChanged(const Framebuffer& framebuffer)
	{
		VkWriteDescriptorSet depthBufferDSWrite;
		
		VkDescriptorImageInfo depthImageInfo =
		{
			/* sampler     */ VK_NULL_HANDLE, //Immutable
			/* imageView   */ framebuffer.GetDepthImageView(),
			/* imageLayout */ VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
		};;
		m_descriptorSet.InitWriteDescriptorSet(depthBufferDSWrite, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, depthImageInfo);
		UpdateDescriptorSets(SingleElementSpan(depthBufferDSWrite));
		
		const float starSize = 0.02f;
		m_starSize = glm::vec2(starSize, starSize * framebuffer.GetHeight() / framebuffer.GetWidth());
	}
	
	void StarRenderer::Render(CommandBuffer& commandBuffer, const TimeManager& timeManager)
	{
		float intensity = timeManager.GetStarIntensityScale();
		if (intensity < 1E-6f)
			return;
		
		m_starShader.Bind(commandBuffer);
		
		VkDescriptorSet descriptorSets[] = { *m_descriptorSet };
		commandBuffer.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_starShader.GetLayout(), 0, descriptorSets);
		
		glm::mat3 rotation(glm::rotate(glm::mat4(), timeManager.GetAccumulatedTime() * 1.5f, glm::vec3(1, 0, 0)));
		
		float pcData[4 * 4];
		pcData[0] = m_starSize.x;
		pcData[1] = m_starSize.y;
		pcData[2] = intensity;
		
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				pcData[4 + i * 4 + j] = rotation[i][j];
			}
		}
		
		commandBuffer.PushConstants(m_starShader.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pcData), pcData);
		
		VkDeviceSize offsets[] = { 0 };
		commandBuffer.BindVertexBuffers(0, 1, &*m_starFieldBuffer, offsets);
		
		commandBuffer.Draw(6, numStars, 0, 0);
	}
}
