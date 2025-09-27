#pragma once
#include "EasyVKStart.h"
//定义vulkan命名空间，把Vulkan中一些基本对象的封装写在其中
namespace vulkan {
	class graphicsBase
	{
		uint32_t apiVersion = VK_API_VERSION_1_0;
		//创建Vulkan实例分为两步：1.确定所需的实例级别的层级或扩展，不检查是否可用（因为后面vkCreateInstance(...)会检查）2.用vkCreateInstance(...)创建Vulkan实例
		//层只有实例级别，扩展既有实例也有设备级别的
		//TODO：实例级别大概是指全局性的，设备级别是与特定设备相关?//////////////////////////////////////////////////
		VkInstance instance;							//Vulkan实例
		std::vector<const char*> instanceLayers;		//实例级别层级组
		std::vector<const char*> instanceExtensions;	//实例级别扩展组

		VkDebugUtilsMessengerEXT debugMessenger;		//用于获取验证层所捕捉到的debug信息

		//
		VkSurfaceKHR surface;							//用于与平台特定的窗口对接。

		//创建逻辑设备步骤：	1.获取物理设备列表 2.检查物理设备是否满足所需的队列族类型，从中选择能满足要求的设备并顺便取得队列族索引 3.确定所需的设备级别扩展，不检查是否可用（因为后面vkCreateDevice(...)会检查）
		//					4.用vkCreateDevice(...)创建逻辑设备，取得队列 5.取得物理设备属性、物理设备内存属性，以备之后使用
		VkPhysicalDevice physicalDevice;									//物理设备
		VkPhysicalDeviceProperties physicalDeviceProperties;				//物理设备属性
		VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;	//物理设备内存属性
		std::vector<VkPhysicalDevice> availablePhysicalDevices;				//可用的物理设备

		VkDevice device;													//逻辑设备
		//有效的索引从0开始，因此使用特殊值VK_QUEUE_FAMILY_IGNORED（为UINT32_MAX）为队列族索引的默认值
		uint32_t queueFamilyIndex_graphics = VK_QUEUE_FAMILY_IGNORED;		//图形队列族索引
		uint32_t queueFamilyIndex_presentation = VK_QUEUE_FAMILY_IGNORED;	//呈现队列族索引
		uint32_t queueFamilyIndex_compute = VK_QUEUE_FAMILY_IGNORED;		//计算队列族索引
		VkQueue queue_graphics;												//图形队列
		VkQueue queue_presentation;											//呈现队列
		VkQueue queue_compute;												//计算队列

		std::vector<const char*> deviceExtensions;							//设备级别扩展组

		//该函数被DeterminePhysicalDevice(...)调用，用于检查物理设备是否满足所需的队列族类型，并将对应的队列族索引返回到queueFamilyIndices，执行成功时直接将索引写入相应成员变量
		VkResult GetQueueFamilyIndices(VkPhysicalDevice physicalDevice, bool enableGraphicsQueue, bool enableComputeQueue, uint32_t(&queueFamilyIndices)[3]) {
			
		}

		std::vector <VkSurfaceFormatKHR> availableSurfaceFormats;	//可用的（表面？）格式组

		VkSwapchainKHR swapchain;									//交换链
		std::vector <VkImage> swapchainImages;						//用作图像的设备内存组
		std::vector <VkImageView> swapchainImageViews;				//图像的使用方式组
		//保存交换链的创建信息以便重建交换链							
		VkSwapchainCreateInfoKHR swapchainCreateInfo = {};			//交换链创建信息

		static graphicsBase singleton;								//graphicsBase单例

		//向instanceLayers或instanceExtensions容器中添加字符串指针，确保不重复
		static void AddLayerOrExtension(std::vector<const char*> container, const char* name)
		{
			for (auto i : container)
			{
				if (!strcmp(i, name))
				{
					return;
				}
			}
			container.emplace_back(name);
		}

		//创建DebugMessenger
		VkResult CreateDebugMessenger() 
		{

		}

		//该函数被CreateSwapchain(...)和RecreateSwapchain()调用
		VkResult CreateSwapchain_Internal() {
		}

		graphicsBase() = default;
		graphicsBase(graphicsBase&&) = delete;	//删除移动构造
		~graphicsBase()
		{

		}
	public:
		//获取Vulkan版本
		uint32_t ApiVersion() const {
			return apiVersion;
		}
		//使用Vulkan的最新版本
		VkResult UseLatestApiVersion() {
			//可以使用vkEnumerateInstanceVersion()取得当前运行环境所支持的最新Vulkan版本，但Vulkan1.0版本不支持该函数，所以使用vkGetInstanceProcAddr()尝试取得该函数的指针，
			//若返回非空指针，则说明可以执行该函数且Vulkan版本至少为1.1，否则说明当前运行环境中Vulkan的最高版本为1.0。
			if (vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"))
			{
				return vkEnumerateInstanceVersion(&apiVersion);
			}
			return VK_SUCCESS;
		}

		//获取Vulkan实例
		VkInstance Instance() const
		{
			return instance;
		}
		//获取实例级别层级组
		const std::vector<const char*> InstanceLayers() const
		{
			return instanceLayers;
		}
		//获取实例级别扩展组
		const std::vector<const char*> InstanceExtensions() const
		{
			return instanceExtensions;
		}
		//添加实例级别层级
		void AddInstanceLayer(const char* name)
		{
			AddLayerOrExtension(instanceLayers, name);
		}
		//添加实例级别扩展
		void AddInstanceExtension(const char* name)
		{
			AddLayerOrExtension(instanceExtensions, name);
		}
		//创建Vulkan实例
		VkResult CreateInstance(VkInstanceCreateFlags flags = 0) 
		{
		//这段代码意为：仅在编译选项为DEBUG时，在instanceLayers和instanceExtensions尾部加上所需的名称(即DebugMessenger)。
		#ifdef NDEBUG
			AddInstanceLayer("VK_LAYER_KHRONOS_validation");
			AddInstanceExtension(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
		#endif
			VkApplicationInfo applicationInfo;
			applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			applicationInfo.apiVersion = apiVersion;
			VkInstanceCreateInfo instanceCreateInfo;
			instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			instanceCreateInfo.flags = flags;
			instanceCreateInfo.pApplicationInfo = &applicationInfo;
			instanceCreateInfo.enabledLayerCount = uint32_t(instanceLayers.size());
			instanceCreateInfo.ppEnabledLayerNames = instanceLayers.data();
			instanceCreateInfo.enabledExtensionCount = uint32_t(instanceExtensions.size());
			instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
			if (VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance))
			{
				std::cout << "[ graphicsBase ] ERROR\nFailed to create a vulkan instance!\nError code: " << int32_t(result) << std::endl;
				return result;
			}
			std::cout << "Vulkan API Version: " << VK_VERSION_MAJOR(apiVersion) << "." << VK_VERSION_MINOR(apiVersion) << "." << VK_VERSION_PATCH(apiVersion) << std::endl;
		#ifndef NDEBUG
			//创建完Vulkan实例后紧接着创建debug messenger
			CreateDebugMessenger();
		#endif
			return VK_SUCCESS;
		}
		//以下函数用于创建Vulkan实例失败后
		/*检查并去除不可用实例级别层级（将不可用层级设置为nullptr），返回修改后的层级组*/
		VkResult CheckInstanceLayers(std::vector<const char*> layersToCheck)
		{

		}
		//设置实例级别层级
		void InstanceLayers(const std::vector<const char*>& layerNames) 
		{
			instanceLayers = layerNames;
		}
		//检查并去除不可用实例级别扩展（将不可用扩展设置为nullptr），返回修改后的扩展组
		VkResult CheckInstanceExtensions(std::vector<const char*> extensionsToCheck, const char* layerName = nullptr) const 
		{

		}
		//设置实例级别扩展
		void InstanceExtensions(const std::vector<const char*>& extensionNames) 
		{
			instanceExtensions = extensionNames;
		}

		//获取window surface
		VkSurfaceKHR Surface() const {
			return surface;
		}
		//该函数用于选择物理设备前
		void Surface(VkSurfaceKHR surface) {
			if (!this->surface)
				this->surface = surface;
		}

		//获取物理设备
		VkPhysicalDevice PhysicalDevice() const 
		{
			return physicalDevice;
		}
		//获取物理设备属性
		const VkPhysicalDeviceProperties& PhysicalDeviceProperties() const {
			return physicalDeviceProperties;
		}
		//获取物理设备内存属性
		const VkPhysicalDeviceMemoryProperties& PhysicalDeviceMemoryProperties() const {
			return physicalDeviceMemoryProperties;
		}
		//获取可用的物理设备（索引）
		VkPhysicalDevice AvailablePhysicalDevice(uint32_t index) const {
			return availablePhysicalDevices[index];
		}
		//获取可用的物理设备数
		uint32_t AvailablePhysicalDeviceCount() const {
			return uint32_t(availablePhysicalDevices.size());
		}

		//获取逻辑设备
		VkDevice Device() const {
			return device;
		}
		//获取图形队列族索引
		uint32_t QueueFamilyIndex_Graphics() const {
			return queueFamilyIndex_graphics;
		}
		//获取呈现队列族索引
		uint32_t QueueFamilyIndex_Presentation() const {
			return queueFamilyIndex_presentation;
		}
		//获取计算队列族索引
		uint32_t QueueFamilyIndex_Compute() const {
			return queueFamilyIndex_compute;
		}
		//获取图形队列
		VkQueue Queue_Graphics() const {
			return queue_graphics;
		}
		//获取呈现队列
		VkQueue Queue_Presentation() const {
			return queue_presentation;
		}
		//获取计算队列
		VkQueue Queue_Compute() const {
			return queue_compute;
		}

		//获取设备级别扩展组
		const std::vector<const char*>& DeviceExtensions() const {
			return deviceExtensions;
		}

		//该函数用于创建逻辑设备前
		void AddDeviceExtension(const char* extensionName) {
			AddLayerOrExtension(deviceExtensions, extensionName);
		}
		//该函数用于获取物理设备
		VkResult GetPhysicalDevices() {
		}
		//该函数用于指定所用物理设备并调用GetQueueFamilyIndices(...)取得队列族索引
		VkResult DeterminePhysicalDevice(uint32_t deviceIndex = 0, bool enableGraphicsQueue = true, bool enableComputeQueue = true) {
		}
		//该函数用于创建逻辑设备，并取得队列
		VkResult CreateDevice(VkDeviceCreateFlags flags = 0) {
		}
		//以下函数用于创建逻辑设备失败后
		/*检查并去除不可用设备级别扩展（将不可用扩展设置为nullptr），返回修改后的扩展组*/
		VkResult CheckDeviceExtensions(std::vector<const char*> extensionsToCheck, const char* layerName = nullptr) const {
		}
		//设置实例级别扩展
		void DeviceExtensions(const std::vector<const char*>& extensionNames) {
			deviceExtensions = extensionNames;
		}
		
		//获取可用的（表面？）格式（索引）
		const VkFormat& AvailableSurfaceFormat(uint32_t index) const {
			return availableSurfaceFormats[index].format;
		}
		//获取可用的（表面？）颜色空间
		const VkColorSpaceKHR& AvailableSurfaceColorSpace(uint32_t index) const {
			return availableSurfaceFormats[index].colorSpace;
		}
		//获取可用的（表面？）格式数
		uint32_t AvailableSurfaceFormatCount() const {
			return uint32_t(availableSurfaceFormats.size());
		}

		//获取交换链
		VkSwapchainKHR Swapchain() const {
			return swapchain;
		}
		//获取索引处的图像
		VkImage SwapchainImage(uint32_t index) const {
			return swapchainImages[index];
		}
		//获取索引处图像的使用方式
		VkImageView SwapchainImageView(uint32_t index) const {
			return swapchainImageViews[index];
		}
		//获取图像数
		uint32_t SwapchainImageCount() const {
			return uint32_t(swapchainImages.size());
		}
		//获取交换链创建信息
		const VkSwapchainCreateInfoKHR& SwapchainCreateInfo() const {
			return swapchainCreateInfo;
		}

		//获取可用的（表面？）格式
		VkResult GetSurfaceFormats() {
		}
		//设置可用的（表面？）格式
		VkResult SetSurfaceFormat(VkSurfaceFormatKHR surfaceFormat) {
		}
		//该函数用于创建交换链
		VkResult CreateSwapchain(bool limitFrameRate = true, VkSwapchainCreateFlagsKHR flags = 0) {
		}
		//该函数用于重建交换链
		VkResult RecreateSwapchain() {
		}

		//获取graphicsBase单例
		static graphicsBase& Base()
		{
			return singleton;
		}
	};
	//静态变量初始化
	graphicsBase graphicsBase::singleton;//
}