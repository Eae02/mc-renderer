#include "instance.h"
#include "func.h"
#include "vkutils.h"
#include "swapchain.h"
#include "../utils.h"

#include <SDL2/SDL_vulkan.h>
#include <gsl/span>
#include <vector>
#include <memory>
#include <cstring>
#include <algorithm>
#include <sstream>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace MCR
{
	//Declared in func.cpp
	void LoadGlobalVulkanFunctions();
	void LoadInstanceVulkanFunctions(VkInstance instance);
	void LoadDeviceVulkanFunctions(VkDevice device);
	
	static const char* theDeviceExtensions[] = 
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	
	VulkanInstance vulkan;
	
	static std::unique_ptr<Queue> queueInstances[QUEUE_FAMILY_COUNT];
	static VkHandle<VkCommandPool> stdCommandPoolInstances[QUEUE_FAMILY_COUNT] = { };
	
#ifdef MCR_DEBUG
	static VkDebugReportCallbackEXT messageCallback = VK_NULL_HANDLE;
	
	VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
	                                             uint64_t srcObject, size_t location, int32_t msgCode,
	                                             const char* pLayerPrefix, const char* pMsg, void* pUserData)
	{
		const int32_t ignoredMessages[] = { 0, 49, 13, 10 };
		
		if (std::find(MAKE_RANGE(ignoredMessages), msgCode) != std::end(ignoredMessages))
			return VK_FALSE;
		
		if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT || msgCode == 1)
		{
			Log(pLayerPrefix, "[", msgCode, "]: ", pMsg);
			return VK_TRUE;
		}
		
		Log("VK ", pLayerPrefix, "[", msgCode, "]", pMsg);
		
		return VK_FALSE;
	}
#endif
	
	inline bool CanUsePhysicalDevice(VkPhysicalDevice device)
	{
		//Queries supported extensions
		uint32_t numExtensions = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &numExtensions, nullptr);
		if (numExtensions == 0)
			return false;
		std::vector<VkExtensionProperties> availableExtensions(numExtensions);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &numExtensions, availableExtensions.data());
		
		//Checks if all required extensions are supported
		for (const char* extension : theDeviceExtensions)
		{
			bool supported = false;
			for (const VkExtensionProperties& avExtension : availableExtensions)
			{
				if (std::strcmp(avExtension.extensionName, extension) == 0)
				{
					supported = true;
					break;
				}
			}
			
			if (!supported)
				return false;
		}
		
		VkPhysicalDeviceFeatures features = { };
		vkGetPhysicalDeviceFeatures(device, &features);
		
		return features.fullDrawIndexUint32 == VK_TRUE && features.samplerAnisotropy == VK_TRUE &&
		       features.drawIndirectFirstInstance == VK_TRUE && features.depthClamp == VK_TRUE;
	}
	
	inline bool GetQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t* queueFamiliesOut)
	{
		// ** Enumerates queue families for this device **
		uint32_t numQueueFamilies = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &numQueueFamilies, nullptr);
		if (numQueueFamilies == 0)
			return false;
		
		std::vector<VkQueueFamilyProperties> queueFamilyProperties(numQueueFamilies);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &numQueueFamilies, queueFamilyProperties.data());
		
		bool familiesFound[QUEUE_FAMILY_COUNT] = { };
		bool familiesAlsoSupportGraphics[QUEUE_FAMILY_COUNT] = { };
		
		for (uint32_t i = 0; i < numQueueFamilies; i++)
		{
			if (queueFamilyProperties[i].queueCount <= 0)
				continue;
			
			bool supportsGraphics = queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
			bool supportsCompute = queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
			
			auto MaybeUseQueueFamily = [&] (int type)
			{
				if (!familiesFound[type] || (!supportsGraphics && familiesAlsoSupportGraphics[type]))
				{
					familiesFound[type] = true;
					familiesAlsoSupportGraphics[type] = supportsGraphics;
					queueFamiliesOut[type] = i;
				}
			};
			
			//Checks for graphics queues (which must also be able to do compute and present)
			if (supportsGraphics && supportsCompute)
			{
				VkBool32 supportsPresentation;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supportsPresentation);
				
				if (supportsPresentation)
				{
					MaybeUseQueueFamily(QUEUE_FAMILY_GRAPHICS);
				}
			}
			
			//Checks for compute queues
			if (supportsCompute)
			{
				MaybeUseQueueFamily(QUEUE_FAMILY_COMPUTE);
			}
			
			//Checks for transfer queues
			if (queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				MaybeUseQueueFamily(QUEUE_FAMILY_TRANSFER);
			}
		}
		
		if (!familiesFound[QUEUE_FAMILY_GRAPHICS] || !familiesFound[QUEUE_FAMILY_COMPUTE])
		{
			return false;
		}
		
		if (!familiesFound[QUEUE_FAMILY_TRANSFER])
		{
			queueFamiliesOut[QUEUE_FAMILY_TRANSFER] = queueFamiliesOut[QUEUE_FAMILY_GRAPHICS];
		}
		
		return true;
	}
	
	inline VkPhysicalDevice FindPhysicalDevice(VkSurfaceKHR surface, uint32_t* queueFamiliesOut)
	{
		// ** Enumerates physical devices **
		uint32_t numPhysicalDevices;
		if (vkEnumeratePhysicalDevices(vulkan.instance, &numPhysicalDevices, nullptr) != VK_SUCCESS ||
		    numPhysicalDevices == 0)
		{
			return VK_NULL_HANDLE;
		}
		
		std::vector<VkPhysicalDevice> physicalDevices(numPhysicalDevices);
		if (vkEnumeratePhysicalDevices(vulkan.instance, &numPhysicalDevices, physicalDevices.data()) != VK_SUCCESS)
		{
			return VK_NULL_HANDLE;
		}
		
		// ** Searches for a supported physical device **
		for (VkPhysicalDevice physicalDevice : physicalDevices)
		{
			if (CanUsePhysicalDevice(physicalDevice) && GetQueueFamilies(physicalDevice, surface, queueFamiliesOut))
			{
				return physicalDevice;
			}
		}
		
		return VK_NULL_HANDLE;
	}
	
	inline std::vector<const char*> GetSupportedValidationLayers()
	{
		std::vector<const char*> supportedLayers;
		
#ifdef MCR_DEBUG
		// ** Enumerates validation layers **
		uint32_t numLayerProperties;
		vkEnumerateInstanceLayerProperties(&numLayerProperties, nullptr);
		std::vector<VkLayerProperties> layerProperties(numLayerProperties);
		vkEnumerateInstanceLayerProperties(&numLayerProperties, layerProperties.data());
		
		const char* validationLayers[] = 
		{
			"VK_LAYER_LUNARG_core_validation",
			"VK_LAYER_LUNARG_parameter_validation",
			"VK_LAYER_GOOGLE_unique_objects"
		};
		
		// ** Checks that all validation layers are supported **
		for (const char* layer : validationLayers)
		{
			auto pos = std::find_if(MAKE_RANGE(layerProperties), [layer] (const VkLayerProperties& props)
			{
				return std::strcmp(layer, props.layerName) == 0;
			});
			
			if (pos == layerProperties.end())
			{
				Log("Vulkan validation layer '", layer, "' is not supported.");
			}
			else
			{
				supportedLayers.push_back(layer);
			}
		}
#endif
		
		return supportedLayers;
	}
	
	inline bool InstanceExtensionsSupported(gsl::span<const char*> extensions)
	{
		uint32_t numExtensionProps;
		CheckResult(vkEnumerateInstanceExtensionProperties(nullptr, &numExtensionProps, nullptr));
		std::vector<VkExtensionProperties> extensionProps(numExtensionProps);
		CheckResult(vkEnumerateInstanceExtensionProperties(nullptr, &numExtensionProps, extensionProps.data()));
		
		for (const char* extension : extensions)
		{
			auto pos = std::find_if(MAKE_RANGE(extensionProps), [extension] (const VkExtensionProperties& properties)
			{
				return std::strcmp(properties.extensionName, extension) == 0;
			});
			
			if (pos == extensionProps.end())
				return false;
		}
		
		return true;
	}
	
	void InitializeVulkan(SDL_Window* window)
	{
		vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
		
		LoadGlobalVulkanFunctions();
		
		VkApplicationInfo applicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
		applicationInfo.pApplicationName = "EAE Minecraft Renderer";
		applicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
		
		// ** Prepares a list of required extensions **
		unsigned int numInstanceExtensions;
		SDL_Vulkan_GetInstanceExtensions(window, &numInstanceExtensions, nullptr);
		std::vector<const char*> instanceExtensions(numInstanceExtensions);
		SDL_Vulkan_GetInstanceExtensions(window, &numInstanceExtensions, instanceExtensions.data());
		
#ifdef MCR_DEBUG
		instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif
		
		if (!InstanceExtensionsSupported(instanceExtensions))
		{
			throw std::runtime_error("Not all required instance extensions are supported.");
		}
		
		std::vector<const char*> validationLayers = GetSupportedValidationLayers();
		
		// ** Creates the instance **
		VkInstanceCreateInfo instanceCI = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		instanceCI.pApplicationInfo = &applicationInfo;
		instanceCI.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		instanceCI.ppEnabledLayerNames = validationLayers.data();
		instanceCI.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
		instanceCI.ppEnabledExtensionNames = instanceExtensions.data();
		
		CheckResult(vkCreateInstance(&instanceCI, nullptr, &vulkan.instance));
		
		LoadInstanceVulkanFunctions(vulkan.instance);
		
		// ** Creates the debug callback if running in debug mode **
#ifdef MCR_DEBUG
		VkDebugReportCallbackCreateInfoEXT debugCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };
		debugCreateInfo.pfnCallback = &DebugCallback;
		debugCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
		                        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		
		auto vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
		        vkGetInstanceProcAddr(vulkan.instance, "vkCreateDebugReportCallbackEXT"));
		
		vkCreateDebugReportCallbackEXT(vulkan.instance, &debugCreateInfo, nullptr, &messageCallback);
#endif
		
		if (!SDL_Vulkan_CreateSurface(window, vulkan.instance, &vulkan.surface))
		{
			throw std::runtime_error(SDL_GetError());
		}
		
		vulkan.physicalDevice = FindPhysicalDevice(vulkan.surface, vulkan.queueFamilies);
		if (vulkan.physicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("No supported vulkan device available.");
		}
		
		// ** Gets physical device limits **
		VkPhysicalDeviceProperties physicalDeviceProperties = { };
		vkGetPhysicalDeviceProperties(vulkan.physicalDevice, &physicalDeviceProperties);
		vulkan.limits.uniformBufferOffsetAlignment = physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
		vulkan.limits.storageBufferOffsetAlignment = physicalDeviceProperties.limits.minStorageBufferOffsetAlignment;
		vulkan.limits.timestampMillisecondPeriod = physicalDeviceProperties.limits.timestampPeriod * 1E-6f;
		
		Log("Using vulkan device: ", physicalDeviceProperties.deviceName);
		
		static const float queuePriorities[1] = { 1.0f };
		
		// ** Initializes DeviceQueueCreate info structures **
		uint32_t numQueueCreateInfos = 0;
		VkDeviceQueueCreateInfo queueCreateInfos[QUEUE_FAMILY_COUNT];
		for (uint32_t queueFamily : vulkan.queueFamilies)
		{
			//Checks to see if a queue create info has already been appended for this queue family.
			bool exists = std::any_of(queueCreateInfos, queueCreateInfos + numQueueCreateInfos,
			                          [&] (const VkDeviceQueueCreateInfo& queueCI)
			                          {
			                             return queueCI.queueFamilyIndex == queueFamily;
			                          });
			
			if (!exists)
			{
				queueCreateInfos[numQueueCreateInfos] = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
				queueCreateInfos[numQueueCreateInfos].queueFamilyIndex = queueFamily;
				queueCreateInfos[numQueueCreateInfos].queueCount = 1;
				queueCreateInfos[numQueueCreateInfos].pQueuePriorities = queuePriorities;
				numQueueCreateInfos++;
			}
		}
		
		// ** Selects which device features to enable **
		VkPhysicalDeviceFeatures availFeatures = { };
		vkGetPhysicalDeviceFeatures(vulkan.physicalDevice, &availFeatures);
		
		VkPhysicalDeviceFeatures enabledFeatures = { };
		enabledFeatures.fullDrawIndexUint32 = VK_TRUE;
		enabledFeatures.samplerAnisotropy = VK_TRUE;
		enabledFeatures.depthClamp = VK_TRUE;
		enabledFeatures.fillModeNonSolid = VK_TRUE;
		enabledFeatures.multiDrawIndirect = availFeatures.multiDrawIndirect;
		
		vulkan.limits.hasMultiDrawIndirect = enabledFeatures.multiDrawIndirect == VK_TRUE;
		
		// ** Selects a depth format **
		
		//Contains valid depth formats, in order of decreasing preference.
		const VkFormat depthFormats[] = 
		{
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D16_UNORM,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT
		};
		
		const VkFormatFeatureFlags requiredDepthFormatFeatures =
		        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
		
		vulkan.depthFormat = VK_FORMAT_UNDEFINED;
		for (VkFormat format : depthFormats)
		{
			if (CanUseFormat(format, requiredDepthFormatFeatures, VK_IMAGE_TILING_OPTIMAL))
			{
				vulkan.depthFormat = format;
				break;
			}
		}
		
		if (vulkan.depthFormat == VK_FORMAT_UNDEFINED)
		{
			throw std::runtime_error("No supported depth format was found.");
		}
		
		// ** Creates the logical device **
		VkDeviceCreateInfo deviceCI = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		deviceCI.queueCreateInfoCount = numQueueCreateInfos;
		deviceCI.pQueueCreateInfos = queueCreateInfos;
		deviceCI.pEnabledFeatures = &enabledFeatures;
		deviceCI.ppEnabledExtensionNames = theDeviceExtensions;
		deviceCI.enabledExtensionCount = 1;
		
		if (vkCreateDevice(vulkan.physicalDevice, &deviceCI, nullptr, &vulkan.device) != VK_SUCCESS)
		{
			throw std::runtime_error("Could not create vulkan device.");
		}
		
		LoadDeviceVulkanFunctions(vulkan.device);
		
		// ** Creates and assigns queue instances **
		const VkCommandPoolCreateFlags commandPoolFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		uint32_t numCreatedQueues = 0;
		for (int i = 0; i < QUEUE_FAMILY_COUNT; i++)
		{
			auto queueInstancesEnd = queueInstances + numCreatedQueues;
			
			auto it = std::find_if(queueInstances, queueInstancesEnd, [i] (const std::unique_ptr<Queue>& queue)
			{
				return queue->GetFamilyIndex() == vulkan.queueFamilies[i];
			});
			
			if (it != queueInstancesEnd)
			{
				size_t srcQueueIndex = it - queueInstances;
				vulkan.queues[i] = queueInstances[srcQueueIndex].get();
				vulkan.stdCommandPools[i] = *stdCommandPoolInstances[srcQueueIndex];
			}
			else
			{
				queueInstances[numCreatedQueues] = std::make_unique<Queue>(vulkan.queueFamilies[i], 0);
				
				stdCommandPoolInstances[numCreatedQueues] = CreateCommandPool(i, commandPoolFlags);
				
				vulkan.stdCommandPools[i] = *stdCommandPoolInstances[numCreatedQueues];
				vulkan.queues[i] = queueInstances[numCreatedQueues++].get();
			}
		}
		
		VmaAllocatorCreateInfo allocatorCreateInfo = { };
		allocatorCreateInfo.physicalDevice = vulkan.physicalDevice;
		allocatorCreateInfo.device = vulkan.device;
		CheckResult(vmaCreateAllocator(&allocatorCreateInfo, &vulkan.allocator));
		
		SwapChain::Initialize();
	}
	
	void DestroyVulkan()
	{
		vkDeviceWaitIdle(vulkan.device);
		
		SwapChain::Destroy();
		
		vmaDestroyAllocator(vulkan.allocator);
		
		for (int i = 0; i < QUEUE_FAMILY_COUNT; i++)
		{
			vulkan.queues[i] = nullptr;
			queueInstances[i] = nullptr;
			stdCommandPoolInstances[i].Reset();
		}
		
		vkDestroyDevice(vulkan.device, nullptr);
		
		vkDestroySurfaceKHR(vulkan.instance, vulkan.surface, nullptr);
		
		vkDestroyInstance(vulkan.instance, nullptr);
	}
}
