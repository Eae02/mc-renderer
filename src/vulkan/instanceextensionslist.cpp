#include "instanceextensionslist.h"
#include "vk.h"

namespace MCR
{
	InstanceExtensionsList::InstanceExtensionsList()
	{
		uint32_t numExtensionProps;
		CheckResult(vkEnumerateInstanceExtensionProperties(nullptr, &numExtensionProps, nullptr));
		m_extensions.resize(numExtensionProps);
		CheckResult(vkEnumerateInstanceExtensionProperties(nullptr, &numExtensionProps, m_extensions.data()));
	}
	
	bool InstanceExtensionsList::IsExtensionSupported(const char* name) const
	{
		for (const VkExtensionProperties& extensionProperties : m_extensions)
		{
			if (strcmp(extensionProperties.extensionName, name) == 0)
			{
				return true;
			}
		}
		
		return false;
	}
}
