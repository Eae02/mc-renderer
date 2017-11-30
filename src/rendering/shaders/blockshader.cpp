#include "blockshader.h"
#include "../blendstates.h"
#include "../vertex.h"
#include "../windnoiseimage.h"
#include "../../blocks/blockstexturemanager.h"
#include "../../vulkan/setlayoutsmanager.h"

namespace MCR
{
	static const VkDynamicState dynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	
	static const std::string_view setLayouts[] = { "BlockShader_Global", "ShadowSample" };
	
	const Shader::CreateInfo BlockShader::s_createInfo = CreateInfo()
		.SetVertexShaderName("block.vs")
		.SetFragmentShaderName("block.fs")
		.SetDSLayoutNames(setLayouts)
		.SetVertexInputState(&vertexInputState)
		.SetEnableDepthClamp(true)
		.SetCullMode(VK_CULL_MODE_BACK_BIT)
		.SetEnableDepthTest(true)
		.SetEnableDepthWrite(true)
		.SetHasWireframeVariant(true)
		.SetAttachmentBlendStates(SingleElementSpan(BlendStates::noBlending))
		.SetDynamicState(dynamicState);
	
	BlockShader::BlockShader(RenderPassInfo renderPassInfo, const VkDescriptorBufferInfo& renderSettingsBufferInfo)
	    : Shader(renderPassInfo, s_createInfo), m_globalDescriptorSet("BlockShader_Global")
	{
		VkWriteDescriptorSet globalDescriptorWrites[3];
		
		//Render settings buffer
		m_globalDescriptorSet.InitWriteDescriptorSet(globalDescriptorWrites[0], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		                                             renderSettingsBufferInfo);
		
		//Blocks texture array
		const VkDescriptorImageInfo blocksTextureInfo = BlocksTextureManager::GetInstance().GetImageInfo();
		m_globalDescriptorSet.InitWriteDescriptorSet(globalDescriptorWrites[1], 1,
		                                             VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, blocksTextureInfo);
		
		//Wind noise texture
		const VkDescriptorImageInfo windNoiseTextureInfo = WindNoiseImage::GetInstance().GetImageInfo();
		m_globalDescriptorSet.InitWriteDescriptorSet(globalDescriptorWrites[2], 2,
		                                             VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, windNoiseTextureInfo);
		
		UpdateDescriptorSets(globalDescriptorWrites);
	}
	
	void BlockShader::Bind(CommandBuffer& cb, VkDescriptorSet shadowDescriptorSet, BindModes mode) const
	{
		Shader::Bind(cb, mode);
		
		const VkDescriptorSet descriptorSets[] = { *m_globalDescriptorSet, shadowDescriptorSet };
		
		cb.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, GetLayout(), 0, descriptorSets, { });
	}
}
