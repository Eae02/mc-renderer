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
	
	const Shader::CreateInfo BlockShader::s_createInfo = 
	{
		/* vsName                  */ "block.vs",
		/* gsName                  */ "",
		/* fsName                  */ "block.fs",
		/* setLayoutNames          */ setLayouts,
		/* pushConstantRanges      */ { },
		/* vertexInputState        */ &blockVertexInputState,
		/* topology                */ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		/* viewport                */ { 0, 0, 1, 1, 0, 1 },
		/* scissor                 */ { 0, 0, 1, 1 },
		/* enableDepthClamp        */ true,
		/* cullMode                */ VK_CULL_MODE_BACK_BIT,
		/* frontFace               */ VK_FRONT_FACE_CLOCKWISE,
		/* enableDepthTest         */ true,
		/* enableDepthWrite        */ true,
		/* hasWireframeVariant     */ true,
		/* depthCompareOp          */ VK_COMPARE_OP_LESS,
		/* enableDepthBias         */ false,
		/* depthBiasConstantFactor */ 0.0f,
		/* depthBiasClamp          */ 0.0f,
		/* depthBiasSlopeFactor    */ 0.0f,
		/* attachmentBlendStates   */ SingleElementSpan(BlendStates::noBlending),
		/* dynamicState            */ dynamicState
	};
	
	BlockShader::BlockShader(VkRenderPass renderPass, const VkDescriptorBufferInfo& renderSettingsBufferInfo)
	    : Shader(renderPass, s_createInfo), m_globalDescriptorSet("BlockShader_Global")
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
