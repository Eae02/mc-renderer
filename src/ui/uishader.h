#pragma once

#include "../rendering/shaders/shader.h"

namespace MCR
{
	class UIShader : public Shader
	{
	public:
		inline UIShader(VkRenderPass renderPass)
		    : Shader({ renderPass, 0 }, s_createInfo) { }
		
	private:
		static const CreateInfo s_createInfo;
	};
}
