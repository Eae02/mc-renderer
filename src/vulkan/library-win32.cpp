#include "library.h"

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

inline namespace VKFunctions
{
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
}

namespace MCR
{
	bool LoadVulkanLibrary()
	{
		HINSTANCE library = LoadLibrary("vulkan-1.dll");

		if (!library)
			return false;

		vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(library, "vkGetInstanceProcAddr"));
		return vkGetInstanceProcAddr != nullptr;
	}
}

#endif
