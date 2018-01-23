#include "shadermodules.h"
#include "../../utils.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <fstream>

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
		"block.fs",
		"block-shadow.vs",
		"block-shadow.gs",
		"block-shadow.fs",
		"debug.vs",
		"debug.fs",
		"sky.vs",
		"sky.fs",
		"godrays.vs",
		"godrays-gen.fs",
		"godrays-hblur.fs",
		"ui.vs",
		"ui.fs",
		"water.vs",
		"water-tess.vs",
		"water.tcs",
		"water.tes",
		"water.fs",
		"water-post.fs",
		"caustics.cs",
		"post.vs",
		"post.fs",
		"star.vs",
		"star.fs"
	};
	
	void LoadShaderModules()
	{
		shaderModules.clear();
		
		std::vector<uint32_t> fileBuffer;
		
		for (std::string_view shaderModuleName : shaderModuleNames)
		{
			fs::path path = GetResourcePath() / "spv" / (std::string(shaderModuleName) + ".spv");
			
			size_t fileSize = fs::file_size(path);
			
			fileBuffer.resize(DivRoundUp(fileSize, sizeof(uint32_t)));
			
			{
				std::ifstream stream(path, std::ios::binary);
				if (!stream)
				{
					throw std::runtime_error("Error opening shader for reading: '" + path.string() + "'.");
				}
				
				stream.read(reinterpret_cast<char*>(fileBuffer.data()), fileSize);
			}
			
			const VkShaderModuleCreateInfo createInfo = 
			{
				/* sType    */ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				/* pNext    */ nullptr,
				/* flags    */ 0,
				/* codeSize */ fileBuffer.size() * sizeof(uint32_t),
				/* pCode    */ fileBuffer.data()
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
			Log("Shader module not found '", name, "'.");
			std::terminate();
		}
		
		return *it->m_module;
	}
}
