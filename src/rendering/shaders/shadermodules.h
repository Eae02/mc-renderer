#pragma once

#include "../../vulkan/vk.h"

#include <string_view>

namespace MCR
{
	void LoadShaderModules();
	void DestroyShaderModules();
	
	VkShaderModule GetShaderModule(std::string_view name);
}
