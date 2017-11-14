#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace MCR
{
	class InstanceExtensionsList
	{
	public:
		InstanceExtensionsList();
		
		bool IsExtensionSupported(const char* name) const;
		
	private:
		std::vector<VkExtensionProperties> m_extensions;
	};
}
