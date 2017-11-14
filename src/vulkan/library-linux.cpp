#include "library.h"

#ifdef __linux__

#include <dlfcn.h>

inline namespace VKFunctions
{
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
}

namespace MCR
{
	bool LoadVulkanLibrary()
	{
		void* library = dlopen("libvulkan.so", RTLD_LAZY);
		if (library == nullptr)
			return false;
		
		vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(library, "vkGetInstanceProcAddr"));
		return vkGetInstanceProcAddr != nullptr;
	}
}

#endif
