#pragma once

#include "../rendering/shaders/shader.h"

namespace MCR
{
	class UIShader : public Shader
	{
	public:
		inline UIShader(VkRenderPass renderPass)
		    : Shader(renderPass, s_createInfo) { }
		
	private:
		static const CreateInfo s_createInfo;
	};
}
