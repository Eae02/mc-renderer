#include "instance.h"
#include "func.h"
#include "vkutils.h"
#include "swapchain.h"
#include "platform.h"
#include "../utils.h"
#include "../arguments.h"
#include "instanceextensionslist.h"

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
	
	VulkanInstance vulkan;
	
	static std::unique_ptr<Queue> queueInstances[QUEUE_FAMILY_COUNT];
	static VkHandle<VkCommandPool> stdCommandPoolInstances[QUEUE_FAMILY_COUNT] = { };
	
#ifdef MCR_DEBUG
	static VkDebugReportCallbackEXT messageCallback = VK_NULL_HANDLE;
	
	VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
	                                             uint64_t srcObject, size_t location, int32_t msgCode,
	                                             const char* pLayerPrefix, const char* pMsg, void* pUserData)
	{
		if (msgCode == 11 || msgCode == 12) //False positive when using dedicated allocation extension
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
		
		//Checks that the swapchain extension is supported
		bool hasSwapChainExt = std::any_of(MAKE_RANGE(availableExtensions), [] (const VkExtensionProperties& properties)
		{
			return std::strcmp(properties.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0;
		});
		if (!hasSwapChainExt)
			return false;
		
		VkPhysicalDeviceFeatures features = { };
		vkGetPhysicalDeviceFeatures(device, &features);
		
		return features.fullDrawIndexUint32 == VK_TRUE && features.samplerAnisotropy == VK_TRUE &&
		       features.drawIndirectFirstInstance == VK_TRUE && features.depthClamp == VK_TRUE &&
		       features.geometryShader == VK_TRUE && features.shaderStorageImageExtendedFormats == VK_TRUE &&
		       features.shaderCullDistance == VK_TRUE;
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
		
		if (!familiesFound[QUEUE_FAMILY_TRANSFER] || Arguments::noBackgroundTransfer)
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
		
		if (Arguments::noValidation)
			return supportedLayers;
		
#ifdef MCR_DEBUG
		// ** Enumerates validation layers **
		uint32_t numLayerProperties;
		vkEnumerateInstanceLayerProperties(&numLayerProperties, nullptr);
		std::vector<VkLayerProperties> layerProperties(numLayerProperties);
		vkEnumerateInstanceLayerProperties(&numLayerProperties, layerProperties.data());
		
		const char* validationLayers[] = 
		{
			"VK_LAYER_LUNARG_standard_validation"
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
	
	void InitializeVulkan(SDL_Window* window)
	{
		LoadGlobalVulkanFunctions();
		
		VkApplicationInfo applicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
		applicationInfo.pApplicationName = "EAE Minecraft Renderer";
		applicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
		
		InstanceExtensionsList instanceExtensionsList;
		
		// ** Prepares a list of required extensions **
		const char* platformSurfaceExtension = GetPlatformSurfaceName(window, instanceExtensionsList);
		if (platformSurfaceExtension == nullptr)
		{
			throw std::runtime_error("Unsupported window system");
		}
		
		std::array<const char*, 3> instanceExtensions;
		instanceExtensions[0] = platformSurfaceExtension;
		instanceExtensions[1] = VK_KHR_SURFACE_EXTENSION_NAME;
		uint32_t numInstanceExtensions = 2;
		
#ifdef MCR_DEBUG
		instanceExtensions[numInstanceExtensions++] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
#endif
		
		//Checks that all required instance extensions are supported
		for (uint32_t i = 0; i < numInstanceExtensions; i++)
		{
			if (!instanceExtensionsList.IsExtensionSupported(instanceExtensions[i]))
			{
				std::ostringstream errorMessage;
				errorMessage << "Required instance extension '" << instanceExtensions[i] << "' not supported.";
				throw std::runtime_error(errorMessage.str());
			}
		}
		
		std::vector<const char*> validationLayers = GetSupportedValidationLayers();
		
		// ** Creates the instance **
		VkInstanceCreateInfo instanceCI = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		instanceCI.pApplicationInfo = &applicationInfo;
		instanceCI.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		instanceCI.ppEnabledLayerNames = validationLayers.data();
		instanceCI.enabledExtensionCount = numInstanceExtensions;
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
		
		vulkan.surface = CreateVulkanSurface(window, instanceExtensions[0]);
		
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
		vulkan.limits.maxComputeWorkGroupInvocations = physicalDeviceProperties.limits.maxComputeWorkGroupInvocations;
		std::copy_n(physicalDeviceProperties.limits.maxComputeWorkGroupSize, 3, vulkan.limits.maxComputeWorkGroupSize);
		
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
		enabledFeatures.geometryShader = VK_TRUE;
		enabledFeatures.shaderStorageImageExtendedFormats = VK_TRUE;
		enabledFeatures.shaderCullDistance = VK_TRUE;
		enabledFeatures.multiDrawIndirect = availFeatures.multiDrawIndirect;
		enabledFeatures.tessellationShader = availFeatures.tessellationShader;
		
		if (physicalDeviceProperties.vendorID == 32902)
		{
			enabledFeatures.multiDrawIndirect = VK_FALSE;
		}
		
		vulkan.limits.hasMultiDrawIndirect = false;//enabledFeatures.multiDrawIndirect == VK_TRUE;
		vulkan.limits.hasTessellation = enabledFeatures.tessellationShader == VK_TRUE;
		
		// ** Selects a depth format **
		
		//Contains valid depth stencil formats, in order of decreasing preference.
		const VkFormat depthStencilFormats[] = 
		{
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT
		};
		
		const VkFormatFeatureFlags requiredDepthFormatFeatures =
		        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
		
		vulkan.depthStencilFormat = VK_FORMAT_UNDEFINED;
		for (VkFormat format : depthStencilFormats)
		{
			if (CanUseFormat(format, requiredDepthFormatFeatures, VK_IMAGE_TILING_OPTIMAL))
			{
				vulkan.depthStencilFormat = format;
				break;
			}
		}
		
		if (vulkan.depthStencilFormat == VK_FORMAT_UNDEFINED)
		{
			throw std::runtime_error("No supported depth format was found.");
		}
		
		// ** Selects which device extensions to enable **
		uint32_t numAvailableDeviceExtensions;
		CheckResult(vkEnumerateDeviceExtensionProperties(vulkan.physicalDevice, nullptr, &numAvailableDeviceExtensions,
		                                                 nullptr));
		std::vector<VkExtensionProperties> availableDeviceExtensions(numAvailableDeviceExtensions);
		CheckResult(vkEnumerateDeviceExtensionProperties(vulkan.physicalDevice, nullptr, &numAvailableDeviceExtensions,
		                                                 availableDeviceExtensions.data()));
		
		std::vector<const char*> deviceExtensions;
		
		auto MaybeEnableExtension = [&] (const char* name)
		{
			bool supported = std::any_of(MAKE_RANGE(availableDeviceExtensions),
			                             [&] (const VkExtensionProperties& properties)
			{
				return std::strcmp(properties.extensionName, name) == 0;
			});
			
			if (supported)
			{
				deviceExtensions.push_back(name);
			}
			
			return supported;
		};
		
		MaybeEnableExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		const bool hasDedicatedAllocation = !Arguments::noVkExtensions &&
		        MaybeEnableExtension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME) &&
		        MaybeEnableExtension(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
		
		// ** Creates the logical device **
		VkDeviceCreateInfo deviceCI = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		deviceCI.queueCreateInfoCount = numQueueCreateInfos;
		deviceCI.pQueueCreateInfos = queueCreateInfos;
		deviceCI.pEnabledFeatures = &enabledFeatures;
		deviceCI.ppEnabledExtensionNames = deviceExtensions.data();
		deviceCI.enabledExtensionCount = gsl::narrow<uint32_t>(deviceExtensions.size());
		
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
		
		VmaVulkanFunctions allocatorVulkanFunctions = { };
		allocatorVulkanFunctions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
		allocatorVulkanFunctions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
		allocatorVulkanFunctions.vkAllocateMemory = vkAllocateMemory;
		allocatorVulkanFunctions.vkFreeMemory = vkFreeMemory;
		allocatorVulkanFunctions.vkMapMemory = vkMapMemory;
		allocatorVulkanFunctions.vkUnmapMemory = vkUnmapMemory;
		allocatorVulkanFunctions.vkBindBufferMemory = vkBindBufferMemory;
		allocatorVulkanFunctions.vkBindImageMemory = vkBindImageMemory;
		allocatorVulkanFunctions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
		allocatorVulkanFunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
		allocatorVulkanFunctions.vkCreateBuffer = vkCreateBuffer;
		allocatorVulkanFunctions.vkDestroyBuffer = vkDestroyBuffer;
		allocatorVulkanFunctions.vkCreateImage = vkCreateImage;
		allocatorVulkanFunctions.vkDestroyImage = vkDestroyImage;
		
		VmaAllocatorCreateInfo allocatorCreateInfo = { };
		allocatorCreateInfo.physicalDevice = vulkan.physicalDevice;
		allocatorCreateInfo.device = vulkan.device;
		allocatorCreateInfo.pVulkanFunctions = &allocatorVulkanFunctions;
		
		if (hasDedicatedAllocation)
		{
			allocatorVulkanFunctions.vkGetBufferMemoryRequirements2KHR =
				VK_GET_DEVICE_FUNCTION(vkGetBufferMemoryRequirements2KHR);
			allocatorVulkanFunctions.vkGetImageMemoryRequirements2KHR = 
				VK_GET_DEVICE_FUNCTION(vkGetImageMemoryRequirements2KHR);
			
			allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
		}
		
		CheckResult(vmaCreateAllocator(&allocatorCreateInfo, &vulkan.allocator));
		
		SwapChain::Initialize();
	}
	
	void DestroyVulkan()
	{
		vkDeviceWaitIdle(vulkan.device);
		
		SwapChain::Destroy();
		
		for (int i = 0; i < QUEUE_FAMILY_COUNT; i++)
		{
			vulkan.queues[i] = nullptr;
			queueInstances[i] = nullptr;
			stdCommandPoolInstances[i].Reset();
		}
		
		vmaDestroyAllocator(vulkan.allocator);
		
		vkDestroyDevice(vulkan.device, nullptr);
		
		vkDestroySurfaceKHR(vulkan.instance, vulkan.surface, nullptr);
		
#ifdef MCR_DEBUG
		auto vkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
		        vkGetInstanceProcAddr(vulkan.instance, "vkDestroyDebugReportCallbackEXT"));
		vkDestroyDebugReportCallbackEXT(vulkan.instance, messageCallback, nullptr);
#endif
		
		vkDestroyInstance(vulkan.instance, nullptr);
	}
}
