#pragma once

#include "Platform.h"

#include <vector>
#include <string>

class Window;

class Renderer
{
public:
	Renderer();
	~Renderer();

	Window									*	OpenWindow(uint32_t size_x, uint32_t size_y, std::string name);
	void InitDevice(const VkSurfaceKHR& surface);
	bool										Run();

	const VkInstance							GetVulkanInstance()	const;
	const VkPhysicalDevice						GetVulkanPhysicalDevice() const;
	const VkDevice								GetVulkanDevice() const;
	const VkQueue								GetVulkanQueue() const;
	const uint32_t								GetVulkanGraphicsQueueFamilyIndex() const;
	const VkPhysicalDeviceProperties		&	GetVulkanPhysicalDeviceProperties() const;
	const VkPhysicalDeviceMemoryProperties	&	GetVulkanPhysicalDeviceMemoryProperties() const;
	
private:
	struct QueueFamilyIndices {
		uint32_t graphicsFamily = -1;
		uint32_t presentFamily = -1;

		bool isComplete() const {
			return graphicsFamily >= 0 && presentFamily >= 0;
		}
	};
	void _SetupLayersAndExtensions();

	void CreateInstance();
	void DestroyInstance();

	
	void _DeInitDevice();

	int RateDeviceSuitability(const VkPhysicalDevice& deviceToRate) const;

	void _SetupDebug();
	void _InitDebug();
	void _DeInitDebug();

	VkSurfaceKHR*							surface = VK_NULL_HANDLE;

	VkInstance								_instance = VK_NULL_HANDLE;
	std::vector<VkPhysicalDevice>			_gpu;
	std::vector<QueueFamilyIndices>			_queue_family_indices;
	VkDevice								_device = VK_NULL_HANDLE;
	VkQueue									_queue = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties				_gpu_properties = {};
	VkPhysicalDeviceMemoryProperties		_gpu_memory_properties = {};

	//uint32_t								_graphics_family_index = 0;

	

	std::vector<const char*>				_instance_layers;
	std::vector<const char*>				_instance_extensions;
	std::vector<const char*>				_device_extensions;

	VkDebugReportCallbackEXT				_debug_report = VK_NULL_HANDLE;
	VkDebugReportCallbackCreateInfoEXT		_debug_callback_create_info = {};
};

