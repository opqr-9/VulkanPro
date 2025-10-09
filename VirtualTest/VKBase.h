#pragma once
#include "EasyVKStart.h"
//定义vulkan命名空间，把Vulkan中一些基本对象的封装写在其中
namespace vulkan {
	//默认窗口大小
	constexpr VkExtent2D defaultWindowSize = { 1280, 720 };

//异常处理辅助类result_t
//情况1：根据函数返回值确定是否抛异常
#ifdef VK_RESULT_THROW
	class result_t {
		VkResult result;
	public:
		static void(*callback_throw)(VkResult);
		result_t(VkResult result) :result(result) {}
		result_t(result_t&& other) noexcept :result(other.result) { other.result = VK_SUCCESS; }
		~result_t() noexcept(false) {
			if (uint32_t(result) < VK_RESULT_MAX_ENUM)
				return;
			if (callback_throw)
				callback_throw(result);
			throw result;
		}
		operator VkResult() {
			VkResult result = this->result;
			this->result = VK_SUCCESS;
			return result;
		}
	};
	inline void(*result_t::callback_throw)(VkResult);

	//情况2：若抛弃函数返回值，让编译器发出警告
	#elifdef VK_RESULT_NODISCARD
		struct [[nodiscard]] result_t {
		VkResult result;
		result_t(VkResult result) :result(result) {}
		operator VkResult() const { return result; }
	};
	//在本文件中关闭弃值提醒（因为我懒得做处理）
#pragma warning(disable:4834)
#pragma warning(disable:6031)

//情况3：啥都不干
#else
	using result_t = VkResult;
#endif

	//泛型数组指向类，封装个数和数组地址
	template<typename T>
	class arrayRef {
		T* const pArray = nullptr;
		size_t count = 0;
	public:
		//从空参数构造，count为0
		arrayRef() = default;
		//从单个对象构造，count为1
		arrayRef(T& data) :pArray(&data), count(1) {}
		//从顶级数组构造
		template<typename R>
		arrayRef(R&& range) requires requires(R r) {
			requires std::ranges::contiguous_range<R>;
			requires std::ranges::sized_range<R>;
			requires std::ranges::borrowed_range<R>;
			requires std::convertible_to<decltype(std::ranges::data(r)), T*>
		} :pArray(std::ranges::data(range)), count(std::ranges::size(range)) {}
		//从指针和元素个数构造
		arrayRef(T* pData, size_t elementCount) :pArray(pData), count(elementCount) {}
		//若T带const修饰，兼容从对应的无const修饰版本的arrayRef构造
		arrayRef(const arrayRef<std::remove_const_t<T>>& other) :pArray(other.Pointer()), count(other.Count()) {}
		//Getter
		T* Pointer() const { return pArray; }
		size_t Count() const { return count; }
		//Const Function
		T& operator[](size_t index) const { return pArray[index]; }
		T* begin() const { return pArray; }
		T* end() const { return pArray + count; }
		//Non-const Function
		//禁止复制/移动赋值（arrayRef旨在模拟“对数组的引用”，用处归根结底只是传参，故使其同C++引用的底层地址一样，防止初始化后被修改）
		arrayRef& operator=(const arrayRef&) = delete;
	};

//常用的宏，使用constexpr确保代码正常着色
#ifndef NDEBUG
#define ENABLE_DEBUG_MESSENGER true
#else
#define ENABLE_DEBUG_MESSENGER false
#endif

/*封装Vulkan对象用的宏*/
//用于析构器中销毁Vulkan对象,用相应Destroy函数后将handle设置为VK_NULL_HANDLE,防止重复析构
#define DestroyHandleBy(Func) if (handle) { Func(graphicsBase::Base().Device(), handle, nullptr); handle = VK_NULL_HANDLE; }
//移动构造器中使用的宏，该宏复制来自另一对象的handle后，将另一对象的handle设置为VK_NULL_HANDLE，转移析构权限
#define MoveHandle handle = other.handle; other.handle = VK_NULL_HANDLE;
//该宏定义移动赋值
#define DefineMoveAssignmentOperator(type) type& operator=(type&& other) { this->~type(); MoveHandle; return *this; }
//该宏定义转换函数，通过返回handle将封装类型对象隐式转换到被封装handle的原始类型(相当于实现get())
#define DefineHandleTypeOperator operator decltype(handle)() const { return handle; }
//该宏定义转换函数，用于取得被封装handle的地址
#define DefineAddressFunction const decltype(handle)* Address() const { return &handle; }

//这个宏用来把函数分割成能被多次执行，以及只执行一次的两个部分：
#define ExecuteOnce(...) { static bool executed = false; if (executed) return __VA_ARGS__; executed = true; }
//于是，这个宏能如同函数般被使用，在其之上的部分可以被多次执行，在其之下的部分只会被执行一次，如果在其之下的部分已经执行过了，那么就直接返回括号中的参数。

	//程序不一定有控制台，可能会需要把错误信息输出到信息弹窗
	inline auto& outStream = std::cout; //不是constexpr，因为std::cout具有外部链接(它不是常量表达式，因为它在程序启动时才被构造,constexpr需要编译时就可确定)

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
		//获取物理设备具有的队列族索引
		result_t GetQueueFamilyIndices(VkPhysicalDevice physicalDevice, bool enableGraphicsQueue, bool enableComputeQueue, uint32_t (&queueFamilyIndices)[3]) 
		{
			uint32_t queueFamilyCount;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
			if (!queueFamilyCount)
			{
				return VK_RESULT_MAX_ENUM;
			}
			std::vector<VkQueueFamilyProperties> queueFamilyPropertieses(queueFamilyCount);
			uint32_t& ig = queueFamilyIndices[0];//图形队列族索引
			uint32_t& ip = queueFamilyIndices[0];//呈现队列族索引
			uint32_t& ic = queueFamilyIndices[0];//计算队列族索引
			ig = ip = ic = VK_QUEUE_FAMILY_IGNORED;
			//遍历所有队列族的索引
			for (uint32_t i = 0; i < queueFamilyCount; i++) 
			{
				//这三个VkBool32变量指示是否可获取（指应该被获取且能获取）相应队列族索引
				VkBool32
					//只在enableGraphicsQueue为true时获取支持图形操作的队列族的索引
					supportGraphics = enableGraphicsQueue && queueFamilyPropertieses[i].queueFlags & VK_QUEUE_GRAPHICS_BIT,
					supportPresentation = false,
					//只在enableComputeQueue为true时获取支持计算的队列族的索引
					supportCompute = enableComputeQueue && queueFamilyPropertieses[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
				//只在创建了window surface时获取支持呈现的队列族的索引
				if (surface)
				{
					if (result_t result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportPresentation)) 
					{
						std::cout << "[ graphicsBase ] ERROR\nFailed to determine if the queue family supports presentation!\nError code: " << int32_t(result) << std::endl;
						return result;
					}
				}
				//若某队列族同时支持图形操作和计算
				if (supportGraphics && supportCompute) 
				{
					//若需要呈现，最好是三个队列族索引全部相同
					if (supportPresentation) 
					{
						ig = ip = ic = i;
						break;
					}
					//除非ig和ic都已取得且相同，否则将它们的值覆写为i，以确保两个队列族索引相同
					if (ig != ic || ig == VK_QUEUE_FAMILY_IGNORED)
					{
						ig = ic = i;
					}
					//如果不需要呈现，那么已经可以break了
					if (!surface)
					{
						break;
					}
				}
				//若任何一个队列族索引可以被取得但尚未被取得，将其值覆写为i
				if (supportGraphics && ig == VK_QUEUE_FAMILY_IGNORED)
				{
					ig = i;
				}
				if (supportPresentation && ip == VK_QUEUE_FAMILY_IGNORED)
				{
					ip = i;
				}
				if (supportCompute && ic == VK_QUEUE_FAMILY_IGNORED)
				{
					ic = i;
				}
			}
			//若任何需要被取得的队列族索引尚未被取得，则函数执行失败
			if (ig == VK_QUEUE_FAMILY_IGNORED && enableGraphicsQueue ||
				ip == VK_QUEUE_FAMILY_IGNORED && surface ||
				ic == VK_QUEUE_FAMILY_IGNORED && enableComputeQueue)
				return VK_RESULT_MAX_ENUM;
			//函数执行成功时，将所取得的队列族索引写入到成员变量
			queueFamilyIndex_graphics = ig;
			queueFamilyIndex_presentation = ip;
			queueFamilyIndex_compute = ic;
			return VK_SUCCESS;
		}

		std::vector <VkSurfaceFormatKHR> availableSurfaceFormats;	//可用的（表面？）格式组

		VkSwapchainKHR swapchain;									//交换链
		std::vector <VkImage> swapchainImages;						//用作图像的设备内存组
		std::vector <VkImageView> swapchainImageViews;				//图像的使用方式组
		//保存交换链的创建信息以便重建交换链							
		VkSwapchainCreateInfoKHR swapchainCreateInfo = {};			//交换链创建信息

		std::vector<void(*)()> callbacks_createSwapchain;			//交换链创建回调组
		std::vector<void(*)()> callbacks_destroySwapchain;			//交换链销毁回调组

		std::vector<void(*)()> callbacks_createDevice;				//逻辑设备创建回调组
		std::vector<void(*)()> callbacks_destroyDevice;				//逻辑设备销毁回调组

		static graphicsBase singleton;								//graphicsBase单例

		//向instanceLayers或instanceExtensions容器中添加字符串指针，确保不重复
		static void AddLayerOrExtension(std::vector<const char*>& container, const char* name)
		{
			for (auto& i : container)
			{
				if (!strcmp(name, i))
				{
					return;
				}
			}
			container.emplace_back(name);
		}

		//创建DebugMessenger
		result_t CreateDebugMessenger() 
		{
			//产生bug信息之后调用的回调函数
			static PFN_vkDebugUtilsMessengerCallbackEXT DebugUtilsMessengerCallback = [](
				VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
				VkDebugUtilsMessageTypeFlagsEXT messageTypes,
				const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
				void* pUserData)->VkBool32 {
					std::cout << pCallbackData->pMessage << "\n\n";
					return VK_FALSE;
			};

			//DebugMessenger创建信息
			VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {};
			debugUtilsMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugUtilsMessengerCreateInfo.messageSeverity =
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugUtilsMessengerCreateInfo.messageType =
				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debugUtilsMessengerCreateInfo.pfnUserCallback = DebugUtilsMessengerCallback;

			//DebugMessenger创建函数
			PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger =
				reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
			if (vkCreateDebugUtilsMessenger) 
			{
				//创建DebugMessenger
				result_t result = vkCreateDebugUtilsMessenger(instance, &debugUtilsMessengerCreateInfo, nullptr, &debugMessenger); //
				if (result)
					std::cout << "[ graphicsBase ] ERROR\nFailed to create a debug messenger!\nError code: "<<int32_t(result)<<std::endl;
				return result;
			}
			std::cout << "Enabled instance extensions:\n";
			for (const char* ext : instanceExtensions) {
				std::cout << "\t" << ext << std::endl;
			}
			std::cout << "[ graphicsBase ] ERROR\nFailed to get the function pointer of vkCreateDebugUtilsMessengerEXT!\n";
			//没有合适的错误代码，用VK_RESULT_MAX_ENUM代替，值为INT32_MAX
			return VK_RESULT_MAX_ENUM;
		}

		//该函数被CreateSwapchain(...)和RecreateSwapchain()调用
		result_t CreateSwapchain_Internal()
		{
			if (result_t result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain)) {
				std::cout << "[ graphicsBase ] ERROR\nFailed to create a swapchain!\nError code: "<<int32_t(result)<<std::endl;
				return result;
			}

			//获取交换链图像
			uint32_t swapchainImageCount;
			if (result_t result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr)) {
				std::cout << "[ graphicsBase ] ERROR\nFailed to get the count of swapchain images!\nError code: "<<int32_t(result)<<std::endl;
				return result;
			}
			swapchainImages.resize(swapchainImageCount);
			if (result_t result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data())) {
				std::cout << "[ graphicsBase ] ERROR\nFailed to get swapchain images!\nError code: "<<int32_t(result)<<std::endl;
				return result;
			}

			//创建image view(图像视图)
			swapchainImageViews.resize(swapchainImageCount);
			VkImageViewCreateInfo imageViewCreateInfo = {};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = swapchainCreateInfo.imageFormat;
			imageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			for (size_t i = 0; i < swapchainImageCount; i++) 
			{
				imageViewCreateInfo.image = swapchainImages[i];
				if (result_t result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]))
				{
					std::cout << "[ graphicsBase ] ERROR\nFailed to create a swapchain image view!\nError code: "<<int32_t(result)<<std::endl;
					return result;
				}
			}
			return VK_SUCCESS;
		}
		//执行回调函数组
		static void ExecuteCallbacks(std::vector<void(*)()> callbacks) 
		{
			for (size_t size = callbacks.size(), i = 0; i < size; i++)
			{
				callbacks[i]();
			}
		}

		graphicsBase() = default;
		graphicsBase(graphicsBase&&) = delete;	//删除移动构造
		~graphicsBase()
		{
			//判断是否创建了Vulkan实例
			if (!instance)
			{
				return;
			}
			//有逻辑设备的情况下等待逻辑设备空闲
			if (device) 
			{
				WaitIdle();
				if (swapchain) 
				{
					ExecuteCallbacks(callbacks_destroySwapchain);
					for (auto& i : swapchainImageViews)
					{
						if (i)
						{
							vkDestroyImageView(device, i, nullptr);
						}
					}
					vkDestroySwapchainKHR(device, swapchain, nullptr);
				}
				ExecuteCallbacks(callbacks_destroyDevice);
				vkDestroyDevice(device, nullptr);
			}

			//销毁surface
			if (surface)
			{
				vkDestroySurfaceKHR(instance, surface, nullptr);
			}

			//销毁DebugMessenger
			if (debugMessenger) {
				PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessenger =
					reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
				if (vkDestroyDebugUtilsMessenger)
				{
					vkDestroyDebugUtilsMessenger(instance, debugMessenger, nullptr);
				}
			}

			//销毁Vulkan实例
			vkDestroyInstance(instance, nullptr);
		}
	public:
		//获取Vulkan版本
		uint32_t ApiVersion() const {
			return apiVersion;
		}
		//使用Vulkan的最新版本
		result_t UseLatestApiVersion() {
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
		result_t CreateInstance(VkInstanceCreateFlags flags = 0)
		{
		//这段代码意为：仅在编译选项为DEBUG时，在instanceLayers和instanceExtensions尾部加上所需的名称(即DebugMessenger)。
			if constexpr (ENABLE_DEBUG_MESSENGER)
			{
				AddInstanceLayer("VK_LAYER_KHRONOS_validation");
				AddInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}

			VkApplicationInfo applicationInfo = {};
			applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			applicationInfo.apiVersion = apiVersion;

			VkInstanceCreateInfo instanceCreateInfo = {};
			instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			instanceCreateInfo.flags = flags;
			instanceCreateInfo.pApplicationInfo = &applicationInfo;
			instanceCreateInfo.enabledLayerCount = uint32_t(instanceLayers.size());
			instanceCreateInfo.ppEnabledLayerNames = instanceLayers.data();
			instanceCreateInfo.enabledExtensionCount = uint32_t(instanceExtensions.size());
			instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

			if (result_t result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance))
			{
				std::cout << "[ graphicsBase ] ERROR\nFailed to create a vulkan instance!\nError code: " << int32_t(result) << std::endl;
				return result;
			}
			std::cout << "Vulkan API Version: " << VK_VERSION_MAJOR(apiVersion) << "." << VK_VERSION_MINOR(apiVersion) << "." << VK_VERSION_PATCH(apiVersion) << std::endl;
			if constexpr (ENABLE_DEBUG_MESSENGER)
			{
				//创建完Vulkan实例后紧接着创建debug messenger
				CreateDebugMessenger();
			}
			return VK_SUCCESS;
		}
		//以下函数用于创建Vulkan实例失败后
		/*检查并去除不可用实例级别层级（将不可用层级设置为nullptr），返回修改后的层级组*/
		result_t CheckInstanceLayers(std::span<const char*> layersToCheck)
		{
			uint32_t layerCount;
			std::vector<VkLayerProperties> availableLayers;
			if (result_t result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr))
			{
				std::cout << "[ graphicsBase ] ERROR\nFailed to get the count of instance layers!\n";
				return result;
			}
			if (layerCount)
			{
				availableLayers.resize(layerCount);
				//用vkEnumerateInstanceLayerProperties(...)获取所有可用层的数量及其VkLayerProperties
				if (result_t result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data())) //
				{
					std::cout << "[ graphicsBase ] ERROR\nFailed to enumerate instance layer properties!\nError code: " << int32_t(result) << std::endl;
					return result;
				}
				//对每个需要的层在可用层中遍历查询
				for (auto& i : layersToCheck) //
				{
					bool found = false;
					for (auto& j : availableLayers)
					{
						if (strcmp(i, j.layerName))
						{
							found=true;
							break;
						}
					}
					if (!found)
					{
						i = nullptr;
					}
				}
			}
			else
			{
				for (auto& i : layersToCheck)
				{
					i = nullptr;
				}
			}
			return VK_SUCCESS;
		}
		//设置实例级别层级
		void InstanceLayers(const std::vector<const char*>& layerNames) 
		{
			instanceLayers = layerNames;
		}
		//检查并去除不可用实例级别扩展（将不可用扩展设置为nullptr），返回修改后的扩展组
		result_t CheckInstanceExtensions(std::span<const char*> extensionsToCheck, const char* layerName = nullptr) const
		{
			uint32_t extensionCount;
			std::vector<VkExtensionProperties> availableExtensions;
			if (result_t result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, nullptr))
			{
				layerName ?
					std::cout << "[ graphicsBase ] ERROR\nFailed to get the count of instance extensions!\nLayer name:"<< layerName << std::endl :
					std::cout << "[ graphicsBase ] ERROR\nFailed to get the count of instance extensions!\n";
				return result;
			}
			if (extensionCount) {
				availableExtensions.resize(extensionCount);
				//用vkEnumerateInstanceExtensionProperties(...)获取特定层（layername层，若为nullptr则取得所有默认提供或隐式开启的层的扩展）可用扩展的数量及其VkExtensionProperties
				if (result_t result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, availableExtensions.data())) //
				{
					std::cout << "[ graphicsBase ] ERROR\nFailed to enumerate instance extension properties!\nError code: "<<int32_t(result)<<std::endl;
					return result;
				}
				//对每个需要的扩展在可用扩展中遍历查询
				for (auto& i : extensionsToCheck) 
				{
					bool found = false;
					for (auto& j : availableExtensions)
					{
						if (!strcmp(i, j.extensionName))
						{
							found = true;
							break;
						}
					}
					if (!found)
					{
						i = nullptr;
					}
				}
			}
			else
			{
				for (auto& i : extensionsToCheck)
				{
					i = nullptr;
				}
			}
			return VK_SUCCESS;
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
		result_t GetPhysicalDevices()
		{
			uint32_t deviceCount;
			if (result_t result = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr))
			{
				std::cout << "[ graphicsBase ] ERROR\nFailed to get the count of physical devices!\nError code: "<<int32_t(result)<<std::endl;
				return result;
			}
			if (!deviceCount)
			{
				std::cout << "[ graphicsBase ] ERROR\nFailed to find any physical device supports vulkan!\n";
				abort();
			}
			availablePhysicalDevices.resize(deviceCount);
			result_t result = vkEnumeratePhysicalDevices(instance, &deviceCount, availablePhysicalDevices.data());
			if (result)
				std::cout << "[ graphicsBase ] ERROR\nFailed to enumerate physical devices!\nError code: "<<int32_t(result)<<std::endl;
			return result;
		}
		//该函数用于指定所用物理设备并调用GetQueueFamilyIndices(...)取得队列族索引
		result_t DeterminePhysicalDevice(uint32_t deviceIndex = 0, bool enableGraphicsQueue = true, bool enableComputeQueue = true)
		{
			//定义一个特殊值用于标记一个队列族索引已被找过但未找到
			static constexpr uint32_t notFound = INT32_MAX; //== VK_QUEUE_FAMILY_IGNORED & INT32_MAX
			//定义队列族索引组合的结构体
			struct queueFamilyIndexCombination {
				uint32_t graphics = VK_QUEUE_FAMILY_IGNORED;
				uint32_t presentation = VK_QUEUE_FAMILY_IGNORED;
				uint32_t compute = VK_QUEUE_FAMILY_IGNORED;
			};
			//queueFamilyIndexCombinations用于为各个物理设备保存一份队列族索引组合
			static std::vector<queueFamilyIndexCombination> queueFamilyIndexCombinations(availablePhysicalDevices.size());
			uint32_t& ig = queueFamilyIndexCombinations[deviceIndex].graphics;
			uint32_t& ip = queueFamilyIndexCombinations[deviceIndex].presentation;
			uint32_t& ic = queueFamilyIndexCombinations[deviceIndex].compute;

			//如果有任何队列族索引已被找过但未找到，返回VK_RESULT_MAX_ENUM
			if (ig == notFound && enableGraphicsQueue ||
				ip == notFound && surface ||
				ic == notFound && enableComputeQueue)
				return VK_RESULT_MAX_ENUM;

			//如果有任何队列族索引应被获取但还未被找过
			if (ig == VK_QUEUE_FAMILY_IGNORED && enableGraphicsQueue ||
				ip == VK_QUEUE_FAMILY_IGNORED && surface ||
				ic == VK_QUEUE_FAMILY_IGNORED && enableComputeQueue) {
				uint32_t indices[3];
				result_t result = GetQueueFamilyIndices(availablePhysicalDevices[deviceIndex], enableGraphicsQueue, enableComputeQueue, indices);
				//若GetQueueFamilyIndices(...)返回VK_SUCCESS或VK_RESULT_MAX_ENUM（vkGetPhysicalDeviceSurfaceSupportKHR(...)执行成功但没找齐所需队列族），
				//说明对所需队列族索引已有结论，保存结果到queueFamilyIndexCombinations[deviceIndex]中相应变量
				//应被获取的索引若仍为VK_QUEUE_FAMILY_IGNORED，说明未找到相应队列族，VK_QUEUE_FAMILY_IGNORED（~0u）与INT32_MAX做位与得到的数值等于notFound
				if (result == VK_SUCCESS ||
					result == VK_RESULT_MAX_ENUM) {
					if (enableGraphicsQueue)
						ig = indices[0] & INT32_MAX;
					if (surface)
						ip = indices[1] & INT32_MAX;
					if (enableComputeQueue)
						ic = indices[2] & INT32_MAX;
				}
				//如果GetQueueFamilyIndices(...)执行失败，return
				if (result)
					return result;
			}

			//若以上两个if分支皆不执行，则说明所需的队列族索引皆已被获取，从queueFamilyIndexCombinations[deviceIndex]中取得索引
			else {
				queueFamilyIndex_graphics = enableGraphicsQueue ? ig : VK_QUEUE_FAMILY_IGNORED;
				queueFamilyIndex_presentation = surface ? ip : VK_QUEUE_FAMILY_IGNORED;
				queueFamilyIndex_compute = enableComputeQueue ? ic : VK_QUEUE_FAMILY_IGNORED;
			}
			physicalDevice = availablePhysicalDevices[deviceIndex];
			return VK_SUCCESS;
		}
		//该函数用于创建逻辑设备，并取得队列
		result_t CreateDevice(VkDeviceCreateFlags flags = 0)
		{
			float queuePriority = 1.f;

			VkDeviceQueueCreateInfo queueCreateInfos[3] = {};
			queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfos[0].queueCount = 1;
			queueCreateInfos[0].pQueuePriorities = &queuePriority;
			queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfos[1].queueCount = 1;
			queueCreateInfos[1].pQueuePriorities = &queuePriority;
			queueCreateInfos[2].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfos[2].queueCount = 1;
			queueCreateInfos[2].pQueuePriorities = &queuePriority;

			uint32_t queueCreateInfoCount = 0;
			if (queueFamilyIndex_graphics != VK_QUEUE_FAMILY_IGNORED)
				queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = queueFamilyIndex_graphics;
			if (queueFamilyIndex_presentation != VK_QUEUE_FAMILY_IGNORED &&
				queueFamilyIndex_presentation != queueFamilyIndex_graphics)
				queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = queueFamilyIndex_presentation;
			if (queueFamilyIndex_compute != VK_QUEUE_FAMILY_IGNORED &&
				queueFamilyIndex_compute != queueFamilyIndex_graphics &&
				queueFamilyIndex_compute != queueFamilyIndex_presentation)
				queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = queueFamilyIndex_compute;

			VkPhysicalDeviceFeatures physicalDeviceFeatures;
			vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

			VkDeviceCreateInfo deviceCreateInfo = {};
			deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			deviceCreateInfo.flags = flags;
			deviceCreateInfo.queueCreateInfoCount = queueCreateInfoCount;
			deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
			deviceCreateInfo.enabledExtensionCount = uint32_t(deviceExtensions.size());
			deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
			deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

			if (result_t result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device)) {
				std::cout << "[ graphicsBase ] ERROR\nFailed to create a vulkan logical device!\nError code: " << int32_t(result) << std::endl;
				return result;
			}

			if (queueFamilyIndex_graphics != VK_QUEUE_FAMILY_IGNORED)
				vkGetDeviceQueue(device, queueFamilyIndex_graphics, 0, &queue_graphics);
			if (queueFamilyIndex_presentation != VK_QUEUE_FAMILY_IGNORED)
				vkGetDeviceQueue(device, queueFamilyIndex_presentation, 0, &queue_presentation);
			if (queueFamilyIndex_compute != VK_QUEUE_FAMILY_IGNORED)
				vkGetDeviceQueue(device, queueFamilyIndex_compute, 0, &queue_compute);

			vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);
			//输出所用的物理设备名称
			std::cout << "Renderer: " << physicalDeviceProperties.deviceName << std::endl;
			ExecuteCallbacks(callbacks_createDevice);
			return VK_SUCCESS;
		}
		//以下函数用于创建逻辑设备失败后
		/*检查并去除不可用设备级别扩展（将不可用扩展设置为nullptr），返回修改后的扩展组*/
		result_t CheckDeviceExtensions(std::span<const char*> extensionsToCheck, const char* layerName = nullptr) const {
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

		//获取可用的表面格式
		result_t GetSurfaceFormats()
		{
			uint32_t surfaceFormatCount;
			if (result_t result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr))
			{
				std::cout << "[ graphicsBase ] ERROR\nFailed to get the count of surface formats!\nError code: "<<int32_t(result)<<std::endl;
				return result;
			}
			if (!surfaceFormatCount)
			{
				std::cout << "[ graphicsBase ] ERROR\nFailed to find any supported surface format!\n" << std::endl;;
				abort();
			}
			availableSurfaceFormats.resize(surfaceFormatCount);
			result_t result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, availableSurfaceFormats.data());
			if (result)
			{
				std::cout << "[ graphicsBase ] ERROR\nFailed to get surface formats!\nError code: " << int32_t(result) << std::endl;
			}
			return result;
		}
		//设置可用的表面格式
		result_t SetSurfaceFormat(VkSurfaceFormatKHR surfaceFormat)
		{
			bool formatIsAvailable = false;
			if (!surfaceFormat.format) 
			{
				//如果格式未指定，只匹配色彩空间，图像格式有啥就用啥
				for (auto& i : availableSurfaceFormats)
				{
					if (i.colorSpace == surfaceFormat.colorSpace) 
					{
						swapchainCreateInfo.imageFormat = i.format;
						swapchainCreateInfo.imageColorSpace = i.colorSpace;
						formatIsAvailable = true;
						break;
					}
				}
			}
			else
			{
				//否则匹配格式和色彩空间
				for (auto& i : availableSurfaceFormats)
				{
					if (i.format == surfaceFormat.format && i.colorSpace == surfaceFormat.colorSpace)
					{
						swapchainCreateInfo.imageFormat = i.format;
						swapchainCreateInfo.imageColorSpace = i.colorSpace;
						formatIsAvailable = true;
						break;
					}
				}
			}
			//如果没有符合的格式，恰好有个语义相符的错误代码
			if (!formatIsAvailable)
			{
				return VK_ERROR_FORMAT_NOT_SUPPORTED;
			}
			//如果交换链已存在，调用RecreateSwapchain()重建交换链
			if (swapchain)
			{
				return RecreateSwapchain();
			}
			return VK_SUCCESS;
		}
		//该函数用于创建交换链
		result_t CreateSwapchain(bool limitFrameRate = true, VkSwapchainCreateFlagsKHR flags = 0)
		{
			VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
			if (result_t result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities))
			{
				std::cout << "[ graphicsBase ] ERROR\nFailed to get physical device surface capabilities!\nError code: " << int32_t(result) << std::endl;
				return result;
			}

			swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swapchainCreateInfo.flags = flags;
			swapchainCreateInfo.surface = surface;
			swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapchainCreateInfo.clipped = VK_TRUE;
			//指定交换链图像数量
			swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + (surfaceCapabilities.maxImageCount > surfaceCapabilities.minImageCount);
			//指定交换链图像尺寸
			surfaceCapabilities.currentExtent.width == -1 ?
				VkExtent2D{
					glm::clamp(defaultWindowSize.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
					glm::clamp(defaultWindowSize.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height) } :
				surfaceCapabilities.currentExtent;
			swapchainCreateInfo.imageArrayLayers = 1;
			swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
			//指定处理交换链图像透明通道的方式
			if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
			{
				swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
			}
			else
			{
				for (size_t i = 0; i < 4; i++)
				{
					if (surfaceCapabilities.supportedCompositeAlpha & 1 << i) 
					{
						swapchainCreateInfo.compositeAlpha = VkCompositeAlphaFlagBitsKHR(surfaceCapabilities.supportedCompositeAlpha & 1 << i);
						break;
					}

				}
			}
			//指定图像用途
			swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
			{
				swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			}
			if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			{
				swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			}
			else
			{
				std::cout << "[ graphicsBase ] WARNING\nVK_IMAGE_USAGE_TRANSFER_DST_BIT isn't supported!\n";
			}
			//指定图像格式
			if (availableSurfaceFormats.empty())
			{
				if (result_t result = GetSurfaceFormats())
				{
					return result;
				}
			}
			if (!swapchainCreateInfo.imageFormat)
			{
				//用&&操作符来短路执行
				if (SetSurfaceFormat({ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }) &&
					SetSurfaceFormat({ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })) 
				{
					//如果找不到上述图像格式和色彩空间的组合，那只能有什么用什么，采用availableSurfaceFormats中的第一组
					swapchainCreateInfo.imageFormat = availableSurfaceFormats[0].format;
					swapchainCreateInfo.imageColorSpace = availableSurfaceFormats[0].colorSpace;
					std::cout << "[ graphicsBase ] WARNING\nFailed to select a four-component UNORM surface format!\n";
				}
			}
			//指定图像呈现方式
			uint32_t surfacePresentModeCount;
			if (result_t result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount, nullptr)) {
				std::cout << "[ graphicsBase ] ERROR\nFailed to get the count of surface present modes!\nError code: "<<int32_t(result)<<std::endl;
				return result;
			}
			if (!surfacePresentModeCount)
				std::cout << "[ graphicsBase ] ERROR\nFailed to find any surface present mode!\n",
				abort();
			std::vector<VkPresentModeKHR> surfacePresentModes(surfacePresentModeCount);
			if (result_t result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount, surfacePresentModes.data())) {
				std::cout << "[ graphicsBase ] ERROR\nFailed to get surface present modes!\nError code: "<<int32_t(result);
				return result;
			}
			swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
			if (!limitFrameRate)
			{
				for (size_t i = 0; i < surfacePresentModeCount; i++)
				{
					if (surfacePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) 
					{
						swapchainCreateInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
						break;
					}
				}
			}

			if (result_t result = CreateSwapchain_Internal())
			{
				return result;
			}

			ExecuteCallbacks(callbacks_createSwapchain);

			return VK_SUCCESS;
		}
		//该函数用于重建交换链
		result_t RecreateSwapchain()
		{
			VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
			if (result_t result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities)) {
				std::cout << "[ graphicsBase ] ERROR\nFailed to get physical device surface capabilities!\nError code: "<<int32_t(result)<<std::endl;
				return result;
			}
			if (surfaceCapabilities.currentExtent.width == 0 ||
				surfaceCapabilities.currentExtent.height == 0)
				return VK_SUBOPTIMAL_KHR;
			swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;

			swapchainCreateInfo.oldSwapchain = swapchain;

			//等待图形队列写入完毕和呈现队列读取完毕
			result_t result = vkQueueWaitIdle(queue_graphics);
			//仅在等待图形队列成功，且图形与呈现所用队列不同时等待呈现队列
			if (!result && queue_graphics != queue_presentation)
			{
				result = vkQueueWaitIdle(queue_presentation);
			}
			if (result) 
			{
				std::cout << "[ graphicsBase ] ERROR\nFailed to wait for the queue to be idle!\nError code: "<<int32_t(result)<<std::endl;
				return result;
			}

			ExecuteCallbacks(callbacks_destroySwapchain);

			//销毁原有的image view
			for (auto& i : swapchainImageViews)
				if (i)
					vkDestroyImageView(device, i, nullptr);
			swapchainImageViews.resize(0);

			//创建交换链
			if (result = CreateSwapchain_Internal())
			{
				return result;
			}

			ExecuteCallbacks(callbacks_createSwapchain);

			return VK_SUCCESS;
		}

		//添加交换链创建回调函数
		void AddCallback_CreateSwapchain(void(*function)()) {
			callbacks_createSwapchain.push_back(function);
		}
		//添加交换链销毁回调函数
		void AddCallback_DestroySwapchain(void(*function)()) {
			callbacks_destroySwapchain.push_back(function);
		}
		//添加逻辑设备创建回调函数
		void AddCallback_CreateDevice(void(*function)()) {
			callbacks_createDevice.push_back(function);
		}
		//添加逻辑设备创建回调函数
		void AddCallback_DestroyDevice(void(*function)()) {
			callbacks_destroyDevice.push_back(function);
		}

		//等待逻辑设备空闲
		result_t WaitIdle() const
		{
			result_t result = vkDeviceWaitIdle(device);
			if (result)
			{
				std::cout << "[ graphicsBase ] ERROR\nFailed to wait for the device to be idle!\nError code: " << int32_t(result) << std::endl;
			}
			return result;
		}

		//重建逻辑设备 
		result_t RecreateDevice(VkDeviceCreateFlags flags = 0) {
			//销毁原有的逻辑设备
			if (device) {
				result_t result = WaitIdle();
				if (result != VK_SUCCESS && result != VK_ERROR_DEVICE_LOST)
				{
					return result;
				}
				if (swapchain) {
					//调用销毁交换链时的回调函数
					ExecuteCallbacks(callbacks_destroySwapchain);
					//销毁交换链图像的image view
					for (auto& i : swapchainImageViews)
					{
						if (i)
						{
							vkDestroyImageView(device, i, nullptr);
						}
					}
					swapchainImageViews.resize(0);
					//销毁交换链
					vkDestroySwapchainKHR(device, swapchain, nullptr);
					//重置交换链handle
					swapchain = VK_NULL_HANDLE;
					//重置交换链创建信息
					swapchainCreateInfo = {};
				}
				ExecuteCallbacks(callbacks_destroyDevice);
				vkDestroyDevice(device, nullptr);
				device = VK_NULL_HANDLE;
			}
			//创建新的逻辑设备
			return CreateDevice(flags);
		}


		//获取graphicsBase单例
		static graphicsBase& Base()
		{
			return singleton;
		}

		//运行时终止Vulkan,重置相关参数
		void Terminate() {
			this->~graphicsBase();
			instance = VK_NULL_HANDLE;
			physicalDevice = VK_NULL_HANDLE;
			device = VK_NULL_HANDLE;
			surface = VK_NULL_HANDLE;
			swapchain = VK_NULL_HANDLE;
			swapchainImages.resize(0);
			swapchainImageViews.resize(0);
			swapchainCreateInfo = {};
			debugMessenger = VK_NULL_HANDLE;
		}
	};
	//静态变量初始化
	graphicsBase graphicsBase::singleton;//
}