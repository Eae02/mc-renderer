#include "shadermodules.h"
#include "../../utils.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace MCR
{
	struct ShaderModule
	{
		std::string_view m_name;
		VkHandle<VkShaderModule> m_module;
	};
	
	static std::vector<ShaderModule> shaderModules;
	
	static std::string_view shaderModuleNames[] = 
	{
		"block.vs",
		"block.fs"
	};
	
	void LoadShaderModules()
	{
		shaderModules.clear();
		
		for (std::string_view shaderModuleName : shaderModuleNames)
		{
			fs::path path = GetResourcePath() / "spv" / (std::string(shaderModuleName) + ".spv");
			
			std::vector<char> contents = ReadFileContents(path);
			
			const VkShaderModuleCreateInfo createInfo = 
			{
				/* sType    */ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				/* pNext    */ nullptr,
				/* flags    */ 0,
				/* codeSize */ RoundToNextMultiple(contents.size(), sizeof(uint32_t)),
				/* pCode    */ reinterpret_cast<const uint32_t*>(contents.data())
			};
			
			VkShaderModule module;
			CheckResult(vkCreateShaderModule(vulkan.device, &createInfo, nullptr, &module));
			
			shaderModules.push_back({ shaderModuleName, module });
		}
	}
	
	void DestroyShaderModules()
	{
		shaderModules.clear();
	}
	
	VkShaderModule GetShaderModule(std::string_view name)
	{
		auto it = std::find_if(MAKE_RANGE(shaderModules), [&] (const ShaderModule& module)
		{
			return module.m_name == name;
		});
		
		if (it == shaderModules.end())
		{
			Log("Shader module not found '", name, ".");
			std::terminate();
		}
		
		return *it->m_module;
	}
}
