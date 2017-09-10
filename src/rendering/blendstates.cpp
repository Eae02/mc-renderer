#include "blendstates.h"

namespace MCR
{
	static const VkColorComponentFlags ColorWriteMask =
	        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	
	const VkPipelineColorBlendAttachmentState BlendStates::alphaBlend =
	{
		/* blendEnable         */ VK_TRUE,
		/* srcColorBlendFactor */ VK_BLEND_FACTOR_SRC_ALPHA,
		/* dstColorBlendFactor */ VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		/* colorBlendOp        */ VK_BLEND_OP_ADD,
		/* srcAlphaBlendFactor */ VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		/* dstAlphaBlendFactor */ VK_BLEND_FACTOR_ZERO,
		/* alphaBlendOp        */ VK_BLEND_OP_ADD,
		/* colorWriteMask      */ ColorWriteMask
	};
	
	const VkPipelineColorBlendAttachmentState BlendStates::additive =
	{
		/* blendEnable         */ VK_TRUE,
		/* srcColorBlendFactor */ VK_BLEND_FACTOR_ONE,
		/* dstColorBlendFactor */ VK_BLEND_FACTOR_ONE,
		/* colorBlendOp        */ VK_BLEND_OP_ADD,
		/* srcAlphaBlendFactor */ VK_BLEND_FACTOR_ONE,
		/* dstAlphaBlendFactor */ VK_BLEND_FACTOR_ONE,
		/* alphaBlendOp        */ VK_BLEND_OP_ADD,
		/* colorWriteMask      */ ColorWriteMask
	};
	
	const VkPipelineColorBlendAttachmentState BlendStates::noBlending =
	{
		/* blendEnable         */ VK_FALSE,
		/* srcColorBlendFactor */ VK_BLEND_FACTOR_ONE,
		/* dstColorBlendFactor */ VK_BLEND_FACTOR_ZERO,
		/* colorBlendOp        */ VK_BLEND_OP_ADD,
		/* srcAlphaBlendFactor */ VK_BLEND_FACTOR_ONE,
		/* dstAlphaBlendFactor */ VK_BLEND_FACTOR_ZERO,
		/* alphaBlendOp        */ VK_BLEND_OP_ADD,
		/* colorWriteMask      */ ColorWriteMask
	};
}
