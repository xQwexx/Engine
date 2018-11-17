#include "BUILD_OPTIONS.h"
#include "Platform.h"

#include "Renderer.h"
#include "Shared.h"
#include "Window.h"

#include <cstdlib>
#include <assert.h>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>

Renderer::Renderer()
{
	
	_SetupLayersAndExtensions();
	_SetupDebug();
	CreateInstance();
	_InitDebug();
}

Renderer::~Renderer()
{

	_DeInitDevice();
	_DeInitDebug();
	DestroyInstance();
	
}



const VkInstance Renderer::GetVulkanInstance() const
{
	return _instance;
}

const VkPhysicalDevice Renderer::GetVulkanPhysicalDevice() const
{
	return _gpu.front();
}

const VkDevice Renderer::GetVulkanDevice() const
{
	return _device;
}

const VkQueue Renderer::GetVulkanQueue() const
{
	return _queue;
}

const uint32_t Renderer::GetVulkanGraphicsQueueFamilyIndex() const
{
	return _queue_family_indices.front().graphicsFamily;
}

const VkPhysicalDeviceProperties & Renderer::GetVulkanPhysicalDeviceProperties() const
{
	return _gpu_properties;
}

const VkPhysicalDeviceMemoryProperties & Renderer::GetVulkanPhysicalDeviceMemoryProperties() const
{
	return _gpu_memory_properties;
}

void Renderer::_SetupLayersAndExtensions()
{
	_instance_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	AddRequiredPlatformInstanceExtensions(&_instance_extensions); 
	//_device_extensions.push_back(VK_NVX_raytracing);
	_device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

void Renderer::CreateInstance()
{
	VkApplicationInfo app_info{};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.apiVersion = VK_MAKE_VERSION(1, 1, 85);			// 1.0.2 should work on all vulkan enabled drivers.
	app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	app_info.pApplicationName = "Vulkan API Tutorial Series";

	VkInstanceCreateInfo instance_create_info{};
	instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pApplicationInfo = &app_info;
	instance_create_info.enabledLayerCount = _instance_layers.size();
	instance_create_info.ppEnabledLayerNames = _instance_layers.data();
	instance_create_info.enabledExtensionCount = _instance_extensions.size();
	instance_create_info.ppEnabledExtensionNames = _instance_extensions.data();
	//TODO: EZ kell ide?
	instance_create_info.pNext = &_debug_callback_create_info;

	ErrorCheck(vkCreateInstance(&instance_create_info, nullptr, &_instance));
}

void Renderer::DestroyInstance()
{
	vkDestroyInstance(_instance, nullptr);
	_instance = nullptr;
}

void Renderer::InitDevice(const VkSurfaceKHR &surface)
{
	{
		uint32_t gpu_count = 0;
		vkEnumeratePhysicalDevices(_instance, &gpu_count, nullptr);
		std::vector<VkPhysicalDevice> gpu_list(gpu_count);
		ErrorCheck(vkEnumeratePhysicalDevices(_instance, &gpu_count, gpu_list.data()));

		//Sorrendbe helyezni a gpukat és eltárolja 
		std::multimap<int, std::pair<const VkPhysicalDevice&, QueueFamilyIndices&>, std::greater <int> > rankedDevices;

		for (const auto& currentDevice : gpu_list)
		{
			QueueFamilyIndices indices;

			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(currentDevice, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(currentDevice, &queueFamilyCount, queueFamilies.data());

			int i = 0;
			for (const auto& queueFamily : queueFamilies) {
				if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					indices.graphicsFamily = i;
				}

				VkBool32 presentSupport = false;
				ErrorCheck(vkGetPhysicalDeviceSurfaceSupportKHR(currentDevice, i, surface, &presentSupport));

				if (queueFamily.queueCount > 0 && presentSupport) {
					indices.presentFamily = i;
				}

				if (indices.isComplete()) {
					break;
				}

				i++;
			}

			int score = (indices.isComplete()) ? RateDeviceSuitability(currentDevice) : 0;
			if (score > 0) {
				rankedDevices.insert(std::make_pair(score, std::pair<const VkPhysicalDevice&, QueueFamilyIndices&>(currentDevice, indices)));
			}
		}

		if (rankedDevices.size() > 0){
			for (const auto& gpu : rankedDevices){
				_gpu.push_back(gpu.second.first);
				_queue_family_indices.push_back(gpu.second.second);
			}
		}
		else
		{
			assert(0 && "Vulkan ERROR: Supported graphics card not found.");
			std::exit(-1);
		}
		vkGetPhysicalDeviceProperties(_gpu.front(), &_gpu_properties);
		vkGetPhysicalDeviceMemoryProperties(_gpu.front(), &_gpu_memory_properties);
	}

	float queue_priorities[]{ 1.0f };
	VkDeviceQueueCreateInfo device_queue_create_info{};
	device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	device_queue_create_info.queueFamilyIndex = _queue_family_indices.front().graphicsFamily;
	device_queue_create_info.queueCount = 1;
	device_queue_create_info.pQueuePriorities = queue_priorities;

	VkDeviceCreateInfo device_create_info{};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.queueCreateInfoCount = 1;
	device_create_info.pQueueCreateInfos = &device_queue_create_info;
	device_create_info.enabledExtensionCount = _device_extensions.size();
	device_create_info.ppEnabledExtensionNames = _device_extensions.data();

	ErrorCheck(vkCreateDevice(_gpu.front(), &device_create_info, nullptr, &_device));

	vkGetDeviceQueue(_device, _queue_family_indices.front().graphicsFamily, 0, &_queue);
}

void Renderer::_DeInitDevice()
{
	if (_device == nullptr) vkDestroyDevice(_device, nullptr);
	_device = nullptr;
}

int Renderer::RateDeviceSuitability(const VkPhysicalDevice& deviceToRate) const
{
	int score = 0;

	VkPhysicalDeviceFeatures deviceFeatures;
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(deviceToRate, &deviceProperties);
	vkGetPhysicalDeviceFeatures(deviceToRate, &deviceFeatures);

	if (!deviceFeatures.geometryShader)
	{
		return 0;
	}
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	{
		score += 1000;
	}

	score += deviceProperties.limits.maxImageDimension2D;

	return score;
}


#if BUILD_ENABLE_VULKAN_DEBUG

VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanDebugCallback(
	VkDebugReportFlagsEXT		flags,
	VkDebugReportObjectTypeEXT	obj_type,
	uint64_t					src_obj,
	size_t						location,
	int32_t						msg_code,
	const char *				layer_prefix,
	const char *				msg,
	void *						user_data
)
{
	std::ostringstream stream;
	stream << "VKDBG: ";
	if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
		stream << "INFO: ";
	}
	if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
		stream << "WARNING: ";
	}
	if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
		stream << "PERFORMANCE: ";
	}
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		stream << "ERROR: ";
	}
	if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
		stream << "DEBUG: ";
	}
	stream << "@[" << layer_prefix << "]: ";
	stream << msg << std::endl;
	std::cout << stream.str();

#if defined( _WIN32 )
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		MessageBox(NULL, stream.str().c_str(), "Vulkan Error!", 0);
	}
#endif

	return VK_FALSE;
}

void Renderer::_SetupDebug()
{
	//TODO: Atgondolni hogy _debug_callback_create_info áthelyezni InitDebugba
	_debug_callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	_debug_callback_create_info.pfnCallback = VulkanDebugCallback;
	_debug_callback_create_info.flags =
		//		VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
		VK_DEBUG_REPORT_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_ERROR_BIT_EXT |
		//		VK_DEBUG_REPORT_DEBUG_BIT_EXT |
		0;

	_instance_layers.push_back("VK_LAYER_LUNARG_standard_validation");
	/*
	//	_instance_layers.push_back( "VK_LAYER_LUNARG_threading" );
	_instance_layers.push_back( "VK_LAYER_GOOGLE_threading" );
	_instance_layers.push_back( "VK_LAYER_LUNARG_draw_state" );
	_instance_layers.push_back( "VK_LAYER_LUNARG_image" );
	_instance_layers.push_back( "VK_LAYER_LUNARG_mem_tracker" );
	_instance_layers.push_back( "VK_LAYER_LUNARG_object_tracker" );
	_instance_layers.push_back( "VK_LAYER_LUNARG_param_checker" );
	*/
	_instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

}

PFN_vkCreateDebugReportCallbackEXT		fvkCreateDebugReportCallbackEXT = nullptr;
PFN_vkDestroyDebugReportCallbackEXT		fvkDestroyDebugReportCallbackEXT = nullptr;

void Renderer::_InitDebug()
{
	fvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugReportCallbackEXT");
	fvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugReportCallbackEXT");
	if (nullptr == fvkCreateDebugReportCallbackEXT || nullptr == fvkDestroyDebugReportCallbackEXT) {
		assert(0 && "Vulkan ERROR: Can't fetch debug function pointers.");
		std::exit(-1);
	}

	fvkCreateDebugReportCallbackEXT(_instance, &_debug_callback_create_info, nullptr, &_debug_report);

	//	vkCreateDebugReportCallbackEXT( _instance, nullptr, nullptr, nullptr );
}

void Renderer::_DeInitDebug()
{
	fvkDestroyDebugReportCallbackEXT(_instance, _debug_report, nullptr);
	_debug_report = VK_NULL_HANDLE;
}

#else

void Renderer::_SetupDebug() {};
void Renderer::_InitDebug() {};
void Renderer::_DeInitDebug() {};

#endif // BUILD_ENABLE_VULKAN_DEBUG
