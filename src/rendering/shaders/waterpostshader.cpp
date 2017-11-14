#include "waterpostshader.h"
#include "../blendstates.h"
#include "../causticstexture.h"
#include "../framebuffer.h"

namespace MCR
{
	static const VkDynamicState dynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	
	static const std::string_view setLayouts[] = { "WaterPost", "ShadowSample" };
	
	static const VkPushConstantRange pushConstantRange = { VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float) * 2 };
	
	static const VkStencilOpState stencilOpState =
	{
		/* failOp      */ VK_STENCIL_OP_KEEP,
		/* passOp      */ VK_STENCIL_OP_KEEP,
		/* depthFailOp */ VK_STENCIL_OP_KEEP,
		/* compareOp   */ VK_COMPARE_OP_EQUAL,
		/* compareMask */ 0x1,
		/* writeMask   */ 0x0,
		/* reference   */ 0x0
	};
	
	static const Shader::StencilState stencilState = { stencilOpState, stencilOpState };
	
	const Shader::CreateInfo WaterPostShader::s_createInfo =
	{
		/* vsName                  */ "post.vs",
		/* gsName                  */ "",
		/* fsName                  */ "water-post.fs",
		/* setLayoutNames          */ setLayouts,
		/* pushConstantRanges      */ SingleElementSpan(pushConstantRange),
		/* vertexInputState        */ nullptr,
		/* topology                */ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		/* viewport                */ { 0, 0, 1, 1, 0, 1 },
		/* scissor                 */ { 0, 0, 1, 1 },
		/* enableDepthClamp        */ false,
		/* cullMode                */ VK_CULL_MODE_NONE,
		/* frontFace               */ VK_FRONT_FACE_CLOCKWISE,
		/* enableDepthTest         */ false,
		/* enableDepthWrite        */ true,
		/* stencilState            */ &stencilState,
		/* hasWireframeVariant     */ false,
		/* depthCompareOp          */ VK_COMPARE_OP_LESS,
		/* enableDepthBias         */ false,
		/* depthBiasConstantFactor */ 0.0f,
		/* depthBiasClamp          */ 0.0f,
		/* depthBiasSlopeFactor    */ 0.0f,
		/* attachmentBlendStates   */ SingleElementSpan(BlendStates::noBlending),
		/* dynamicState            */ dynamicState
	};
	
	WaterPostShader::WaterPostShader(Shader::RenderPassInfo renderPassInfo,
	                                 const VkDescriptorBufferInfo& renderSettingsBufferInfo)
	    : Shader(renderPassInfo, s_createInfo), m_descriptorSet("WaterPost")
	{
		VkWriteDescriptorSet renderSettingsDSWrite;
		m_descriptorSet.InitWriteDescriptorSet(renderSettingsDSWrite, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		                                       renderSettingsBufferInfo);
		UpdateDescriptorSets(SingleElementSpan(renderSettingsDSWrite));
	}
	
	void WaterPostShader::SetCausticsTexture(const CausticsTexture& causticsTexture)
	{
		VkWriteDescriptorSet dsWrite;
		
		const VkDescriptorImageInfo imageInfo =
			{
				/* sampler     */ VK_NULL_HANDLE, //Immutable
				/* imageView   */ causticsTexture.GetImageView(),
				/* imageLayout */ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};
		m_descriptorSet.InitWriteDescriptorSet(dsWrite, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfo);
		
		UpdateDescriptorSets(SingleElementSpan(dsWrite));
	}
	
	void WaterPostShader::Bind(CommandBuffer& cb, VkDescriptorSet shadowDescriptorSet,
	                           bool underwater, float waterPlaneY) const
	{
		Shader::Bind(cb);
		
		const VkDescriptorSet descriptorSets[] = { *m_descriptorSet, shadowDescriptorSet };
		
		cb.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, GetLayout(), 0, descriptorSets, { });
		
		float pushConstData[2];
		reinterpret_cast<uint32_t*>(pushConstData)[0] = underwater ? 1 : 0;
		pushConstData[1] = waterPlaneY;
		cb.PushConstants(GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float) * 2, pushConstData);
	}
	
	void WaterPostShader::FramebufferChanged(const Framebuffer& framebuffer)
	{
		VkWriteDescriptorSet descriptorWrites[2];
		
		const VkDescriptorImageInfo colorImageInfo =
			{
				/* sampler     */ VK_NULL_HANDLE, //Immutable
				/* imageView   */ framebuffer.GetColorImageView(),
				/* imageLayout */ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};
		m_descriptorSet.InitWriteDescriptorSet(descriptorWrites[0], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		                                       colorImageInfo);
		
		const VkDescriptorImageInfo depthImageInfo =
			{
				/* sampler     */ VK_NULL_HANDLE, //Immutable
				/* imageView   */ framebuffer.GetDepthImageView(),
				/* imageLayout */ VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
			};
		m_descriptorSet.InitWriteDescriptorSet(descriptorWrites[1], 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		                                       depthImageInfo);
		
		UpdateDescriptorSets(descriptorWrites);
	}
}
