#pragma once
#include "EasyVKStart.h"
//����vulkan�����ռ䣬��Vulkan��һЩ��������ķ�װд������
namespace vulkan {
	class graphicsBase
	{
		uint32_t apiVersion = VK_API_VERSION_1_0;
		//����Vulkanʵ����Ϊ������1.ȷ�������ʵ������Ĳ㼶����չ��������Ƿ���ã���Ϊ����vkCreateInstance(...)���飩2.��vkCreateInstance(...)����Vulkanʵ��
		//��ֻ��ʵ��������չ����ʵ��Ҳ���豸�����
		//TODO��ʵ����������ָȫ���Եģ��豸���������ض��豸���?//////////////////////////////////////////////////
		VkInstance instance;							//Vulkanʵ��
		std::vector<const char*> instanceLayers;		//ʵ������㼶��
		std::vector<const char*> instanceExtensions;	//ʵ��������չ��

		VkDebugUtilsMessengerEXT debugMessenger;		//���ڻ�ȡ��֤������׽����debug��Ϣ

		//
		VkSurfaceKHR surface;							//������ƽ̨�ض��Ĵ��ڶԽӡ�

		//�����߼��豸���裺	1.��ȡ�����豸�б� 2.��������豸�Ƿ���������Ķ��������ͣ�����ѡ��������Ҫ����豸��˳��ȡ�ö��������� 3.ȷ��������豸������չ��������Ƿ���ã���Ϊ����vkCreateDevice(...)���飩
		//					4.��vkCreateDevice(...)�����߼��豸��ȡ�ö��� 5.ȡ�������豸���ԡ������豸�ڴ����ԣ��Ա�֮��ʹ��
		VkPhysicalDevice physicalDevice;									//�����豸
		VkPhysicalDeviceProperties physicalDeviceProperties;				//�����豸����
		VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;	//�����豸�ڴ�����
		std::vector<VkPhysicalDevice> availablePhysicalDevices;				//���õ������豸

		VkDevice device;													//�߼��豸
		//��Ч��������0��ʼ�����ʹ������ֵVK_QUEUE_FAMILY_IGNORED��ΪUINT32_MAX��Ϊ������������Ĭ��ֵ
		uint32_t queueFamilyIndex_graphics = VK_QUEUE_FAMILY_IGNORED;		//ͼ�ζ���������
		uint32_t queueFamilyIndex_presentation = VK_QUEUE_FAMILY_IGNORED;	//���ֶ���������
		uint32_t queueFamilyIndex_compute = VK_QUEUE_FAMILY_IGNORED;		//�������������
		VkQueue queue_graphics;												//ͼ�ζ���
		VkQueue queue_presentation;											//���ֶ���
		VkQueue queue_compute;												//�������

		std::vector<const char*> deviceExtensions;							//�豸������չ��

		//�ú�����DeterminePhysicalDevice(...)���ã����ڼ�������豸�Ƿ���������Ķ��������ͣ�������Ӧ�Ķ������������ص�queueFamilyIndices��ִ�гɹ�ʱֱ�ӽ�����д����Ӧ��Ա����
		//��ȡ�����豸���еĶ���������
		VkResult GetQueueFamilyIndices(VkPhysicalDevice physicalDevice, bool enableGraphicsQueue, bool enableComputeQueue, uint32_t (&queueFamilyIndices)[3]) 
		{
			uint32_t queueFamilyCount;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
			if (!queueFamilyCount)
			{
				return VK_RESULT_MAX_ENUM;
			}
			std::vector<VkQueueFamilyProperties> queueFamilyPropertieses(queueFamilyCount);
			uint32_t& ig = queueFamilyIndices[0];//ͼ�ζ���������
			uint32_t& ip = queueFamilyIndices[0];//���ֶ���������
			uint32_t& ic = queueFamilyIndices[0];//�������������
			ig = ip = ic = VK_QUEUE_FAMILY_IGNORED;
			//�������ж����������
			for (uint32_t i = 0; i < queueFamilyCount; i++) 
			{
				//������VkBool32����ָʾ�Ƿ�ɻ�ȡ��ָӦ�ñ���ȡ���ܻ�ȡ����Ӧ����������
				VkBool32
					//ֻ��enableGraphicsQueueΪtrueʱ��ȡ֧��ͼ�β����Ķ����������
					supportGraphics = enableGraphicsQueue && queueFamilyPropertieses[i].queueFlags & VK_QUEUE_GRAPHICS_BIT,
					supportPresentation = false,
					//ֻ��enableComputeQueueΪtrueʱ��ȡ֧�ּ���Ķ����������
					supportCompute = enableComputeQueue && queueFamilyPropertieses[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
				//ֻ�ڴ�����window surfaceʱ��ȡ֧�ֳ��ֵĶ����������
				if (surface)
				{
					if (VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportPresentation)) 
					{
						std::cout << "[ graphicsBase ] ERROR\nFailed to determine if the queue family supports presentation!\nError code: " << int32_t(result) << std::endl;
						return result;
					}
				}
				//��ĳ������ͬʱ֧��ͼ�β����ͼ���
				if (supportGraphics && supportCompute) 
				{
					//����Ҫ���֣��������������������ȫ����ͬ
					if (supportPresentation) 
					{
						ig = ip = ic = i;
						break;
					}
					//����ig��ic����ȡ������ͬ���������ǵ�ֵ��дΪi����ȷ������������������ͬ
					if (ig != ic || ig == VK_QUEUE_FAMILY_IGNORED)
					{
						ig = ic = i;
					}
					//�������Ҫ���֣���ô�Ѿ�����break��
					if (!surface)
					{
						break;
					}
				}
				//���κ�һ���������������Ա�ȡ�õ���δ��ȡ�ã�����ֵ��дΪi
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
			//���κ���Ҫ��ȡ�õĶ�����������δ��ȡ�ã�����ִ��ʧ��
			if (ig == VK_QUEUE_FAMILY_IGNORED && enableGraphicsQueue ||
				ip == VK_QUEUE_FAMILY_IGNORED && surface ||
				ic == VK_QUEUE_FAMILY_IGNORED && enableComputeQueue)
				return VK_RESULT_MAX_ENUM;
			//����ִ�гɹ�ʱ������ȡ�õĶ���������д�뵽��Ա����
			queueFamilyIndex_graphics = ig;
			queueFamilyIndex_presentation = ip;
			queueFamilyIndex_compute = ic;
			return VK_SUCCESS;
		}

		std::vector <VkSurfaceFormatKHR> availableSurfaceFormats;	//���õģ����棿����ʽ��

		VkSwapchainKHR swapchain;									//������
		std::vector <VkImage> swapchainImages;						//����ͼ����豸�ڴ���
		std::vector <VkImageView> swapchainImageViews;				//ͼ���ʹ�÷�ʽ��
		//���潻�����Ĵ�����Ϣ�Ա��ؽ�������							
		VkSwapchainCreateInfoKHR swapchainCreateInfo = {};			//������������Ϣ

		static graphicsBase singleton;								//graphicsBase����

		//��instanceLayers��instanceExtensions����������ַ���ָ�룬ȷ�����ظ�
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

		//����DebugMessenger
		VkResult CreateDebugMessenger() 
		{
			//����bug��Ϣ֮����õĻص�����
			static PFN_vkDebugUtilsMessengerCallbackEXT DebugUtilsMessengerCallback = [](
				VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
				VkDebugUtilsMessageTypeFlagsEXT messageTypes,
				const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
				void* pUserData)->VkBool32 {
					std::cout << pCallbackData->pMessage << "\n\n";
					return VK_FALSE;
			};
			//DebugMessenger������Ϣ
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
			//DebugMessenger��������
			PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger =
				reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
			if (vkCreateDebugUtilsMessenger) 
			{
				//����DebugMessenger
				VkResult result = vkCreateDebugUtilsMessenger(instance, &debugUtilsMessengerCreateInfo, nullptr, &debugMessenger); //
				if (result)
					std::cout << "[ graphicsBase ] ERROR\nFailed to create a debug messenger!\nError code: "<<int32_t(result)<<std::endl;
				return result;
			}
			std::cout << "Enabled instance extensions:\n";
			for (const char* ext : instanceExtensions) {
				std::cout << "\t" << ext << std::endl;
			}
			std::cout << "[ graphicsBase ] ERROR\nFailed to get the function pointer of vkCreateDebugUtilsMessengerEXT!\n";
			//û�к��ʵĴ�����룬��VK_RESULT_MAX_ENUM���棬ֵΪINT32_MAX
			return VK_RESULT_MAX_ENUM;
		}

		//�ú�����CreateSwapchain(...)��RecreateSwapchain()����
		VkResult CreateSwapchain_Internal() {
		}

		graphicsBase() = default;
		graphicsBase(graphicsBase&&) = delete;	//ɾ���ƶ�����
		~graphicsBase()
		{

		}
	public:
		//��ȡVulkan�汾
		uint32_t ApiVersion() const {
			return apiVersion;
		}
		//ʹ��Vulkan�����°汾
		VkResult UseLatestApiVersion() {
			//����ʹ��vkEnumerateInstanceVersion()ȡ�õ�ǰ���л�����֧�ֵ�����Vulkan�汾����Vulkan1.0�汾��֧�ָú���������ʹ��vkGetInstanceProcAddr()����ȡ�øú�����ָ�룬
			//�����طǿ�ָ�룬��˵������ִ�иú�����Vulkan�汾����Ϊ1.1������˵����ǰ���л�����Vulkan����߰汾Ϊ1.0��
			if (vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"))
			{
				return vkEnumerateInstanceVersion(&apiVersion);
			}
			return VK_SUCCESS;
		}

		//��ȡVulkanʵ��
		VkInstance Instance() const
		{
			return instance;
		}
		//��ȡʵ������㼶��
		const std::vector<const char*> InstanceLayers() const
		{
			return instanceLayers;
		}
		//��ȡʵ��������չ��
		const std::vector<const char*> InstanceExtensions() const
		{
			return instanceExtensions;
		}
		//���ʵ������㼶
		void AddInstanceLayer(const char* name)
		{
			AddLayerOrExtension(instanceLayers, name);
		}
		//���ʵ��������չ
		void AddInstanceExtension(const char* name)
		{
			AddLayerOrExtension(instanceExtensions, name);
		}
		//����Vulkanʵ��
		VkResult CreateInstance(VkInstanceCreateFlags flags = 0) 
		{
		//��δ�����Ϊ�����ڱ���ѡ��ΪDEBUGʱ����instanceLayers��instanceExtensionsβ���������������(��DebugMessenger)��
		#ifndef NDEBUG
			AddInstanceLayer("VK_LAYER_KHRONOS_validation");
			AddInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		#endif
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
			if (VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance))
			{
				std::cout << "[ graphicsBase ] ERROR\nFailed to create a vulkan instance!\nError code: " << int32_t(result) << std::endl;
				return result;
			}
			std::cout << "Vulkan API Version: " << VK_VERSION_MAJOR(apiVersion) << "." << VK_VERSION_MINOR(apiVersion) << "." << VK_VERSION_PATCH(apiVersion) << std::endl;
		#ifndef NDEBUG
			//������Vulkanʵ��������Ŵ���debug messenger
			CreateDebugMessenger();
		#endif
			return VK_SUCCESS;
		}
		//���º������ڴ���Vulkanʵ��ʧ�ܺ�
		/*��鲢ȥ��������ʵ������㼶���������ò㼶����Ϊnullptr���������޸ĺ�Ĳ㼶��*/
		VkResult CheckInstanceLayers(std::vector<const char*> layersToCheck)
		{
			uint32_t layerCount;
			std::vector<VkLayerProperties> availableLayers;
			if (VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr)) 
			{
				std::cout << "[ graphicsBase ] ERROR\nFailed to get the count of instance layers!\n";
				return result;
			}
			if (layerCount)
			{
				availableLayers.resize(layerCount);
				//��vkEnumerateInstanceLayerProperties(...)��ȡ���п��ò����������VkLayerProperties
				if (VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data())) //
				{
					std::cout << "[ graphicsBase ] ERROR\nFailed to enumerate instance layer properties!\nError code: " << int32_t(result) << std::endl;
					return result;
				}
				//��ÿ����Ҫ�Ĳ��ڿ��ò��б�����ѯ
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
		//����ʵ������㼶
		void InstanceLayers(const std::vector<const char*>& layerNames) 
		{
			instanceLayers = layerNames;
		}
		//��鲢ȥ��������ʵ��������չ������������չ����Ϊnullptr���������޸ĺ����չ��
		VkResult CheckInstanceExtensions(std::vector<const char*> extensionsToCheck, const char* layerName = nullptr) const 
		{
			uint32_t extensionCount;
			std::vector<VkExtensionProperties> availableExtensions;
			if (VkResult result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, nullptr)) 
			{
				layerName ?
					std::cout << "[ graphicsBase ] ERROR\nFailed to get the count of instance extensions!\nLayer name:"<< layerName << std::endl :
					std::cout << "[ graphicsBase ] ERROR\nFailed to get the count of instance extensions!\n";
				return result;
			}
			if (extensionCount) {
				availableExtensions.resize(extensionCount);
				//��vkEnumerateInstanceExtensionProperties(...)��ȡ�ض��㣨layername�㣬��Ϊnullptr��ȡ������Ĭ���ṩ����ʽ�����Ĳ����չ��������չ����������VkExtensionProperties
				if (VkResult result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, availableExtensions.data())) //
				{
					std::cout << "[ graphicsBase ] ERROR\nFailed to enumerate instance extension properties!\nError code: "<<int32_t(result)<<std::endl;
					return result;
				}
				//��ÿ����Ҫ����չ�ڿ�����չ�б�����ѯ
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
		//����ʵ��������չ
		void InstanceExtensions(const std::vector<const char*>& extensionNames) 
		{
			instanceExtensions = extensionNames;
		}

		//��ȡwindow surface
		VkSurfaceKHR Surface() const {
			return surface;
		}
		//�ú�������ѡ�������豸ǰ
		void Surface(VkSurfaceKHR surface) {
			if (!this->surface)
				this->surface = surface;
		}

		//��ȡ�����豸
		VkPhysicalDevice PhysicalDevice() const 
		{
			return physicalDevice;
		}
		//��ȡ�����豸����
		const VkPhysicalDeviceProperties& PhysicalDeviceProperties() const {
			return physicalDeviceProperties;
		}
		//��ȡ�����豸�ڴ�����
		const VkPhysicalDeviceMemoryProperties& PhysicalDeviceMemoryProperties() const {
			return physicalDeviceMemoryProperties;
		}
		//��ȡ���õ������豸��������
		VkPhysicalDevice AvailablePhysicalDevice(uint32_t index) const {
			return availablePhysicalDevices[index];
		}
		//��ȡ���õ������豸��
		uint32_t AvailablePhysicalDeviceCount() const {
			return uint32_t(availablePhysicalDevices.size());
		}

		//��ȡ�߼��豸
		VkDevice Device() const {
			return device;
		}
		//��ȡͼ�ζ���������
		uint32_t QueueFamilyIndex_Graphics() const {
			return queueFamilyIndex_graphics;
		}
		//��ȡ���ֶ���������
		uint32_t QueueFamilyIndex_Presentation() const {
			return queueFamilyIndex_presentation;
		}
		//��ȡ�������������
		uint32_t QueueFamilyIndex_Compute() const {
			return queueFamilyIndex_compute;
		}
		//��ȡͼ�ζ���
		VkQueue Queue_Graphics() const {
			return queue_graphics;
		}
		//��ȡ���ֶ���
		VkQueue Queue_Presentation() const {
			return queue_presentation;
		}
		//��ȡ�������
		VkQueue Queue_Compute() const {
			return queue_compute;
		}

		//��ȡ�豸������չ��
		const std::vector<const char*>& DeviceExtensions() const {
			return deviceExtensions;
		}

		//�ú������ڴ����߼��豸ǰ
		void AddDeviceExtension(const char* extensionName) {
			AddLayerOrExtension(deviceExtensions, extensionName);
		}
		//�ú������ڻ�ȡ�����豸
		VkResult GetPhysicalDevices() 
		{
			uint32_t deviceCount;
			if (VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr))
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
			VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, availablePhysicalDevices.data());
			if (result)
				std::cout << "[ graphicsBase ] ERROR\nFailed to enumerate physical devices!\nError code: "<<int32_t(result)<<std::endl;
			return result;
		}
		//�ú�������ָ�����������豸������GetQueueFamilyIndices(...)ȡ�ö���������
		VkResult DeterminePhysicalDevice(uint32_t deviceIndex = 0, bool enableGraphicsQueue = true, bool enableComputeQueue = true) 
		{
			//����һ������ֵ���ڱ��һ�������������ѱ��ҹ���δ�ҵ�
			static constexpr uint32_t notFound = INT32_MAX; //== VK_QUEUE_FAMILY_IGNORED & INT32_MAX
			//���������������ϵĽṹ��
			struct queueFamilyIndexCombination {
				uint32_t graphics = VK_QUEUE_FAMILY_IGNORED;
				uint32_t presentation = VK_QUEUE_FAMILY_IGNORED;
				uint32_t compute = VK_QUEUE_FAMILY_IGNORED;
			};
			//queueFamilyIndexCombinations����Ϊ���������豸����һ�ݶ������������
			static std::vector<queueFamilyIndexCombination> queueFamilyIndexCombinations(availablePhysicalDevices.size());
			uint32_t& ig = queueFamilyIndexCombinations[deviceIndex].graphics;
			uint32_t& ip = queueFamilyIndexCombinations[deviceIndex].presentation;
			uint32_t& ic = queueFamilyIndexCombinations[deviceIndex].compute;

			//������κζ����������ѱ��ҹ���δ�ҵ�������VK_RESULT_MAX_ENUM
			if (ig == notFound && enableGraphicsQueue ||
				ip == notFound && surface ||
				ic == notFound && enableComputeQueue)
				return VK_RESULT_MAX_ENUM;

			//������κζ���������Ӧ����ȡ����δ���ҹ�
			if (ig == VK_QUEUE_FAMILY_IGNORED && enableGraphicsQueue ||
				ip == VK_QUEUE_FAMILY_IGNORED && surface ||
				ic == VK_QUEUE_FAMILY_IGNORED && enableComputeQueue) {
				uint32_t indices[3];
				VkResult result = GetQueueFamilyIndices(availablePhysicalDevices[deviceIndex], enableGraphicsQueue, enableComputeQueue, indices);
				//��GetQueueFamilyIndices(...)����VK_SUCCESS��VK_RESULT_MAX_ENUM��vkGetPhysicalDeviceSurfaceSupportKHR(...)ִ�гɹ���û������������壩��
				//˵��������������������н��ۣ���������queueFamilyIndexCombinations[deviceIndex]����Ӧ����
				//Ӧ����ȡ����������ΪVK_QUEUE_FAMILY_IGNORED��˵��δ�ҵ���Ӧ�����壬VK_QUEUE_FAMILY_IGNORED��~0u����INT32_MAX��λ��õ�����ֵ����notFound
				if (result == VK_SUCCESS ||
					result == VK_RESULT_MAX_ENUM) {
					if (enableGraphicsQueue)
						ig = indices[0] & INT32_MAX;
					if (surface)
						ip = indices[1] & INT32_MAX;
					if (enableComputeQueue)
						ic = indices[2] & INT32_MAX;
				}
				//���GetQueueFamilyIndices(...)ִ��ʧ�ܣ�return
				if (result)
					return result;
			}

			//����������if��֧�Բ�ִ�У���˵������Ķ������������ѱ���ȡ����queueFamilyIndexCombinations[deviceIndex]��ȡ������
			else {
				queueFamilyIndex_graphics = enableGraphicsQueue ? ig : VK_QUEUE_FAMILY_IGNORED;
				queueFamilyIndex_presentation = surface ? ip : VK_QUEUE_FAMILY_IGNORED;
				queueFamilyIndex_compute = enableComputeQueue ? ic : VK_QUEUE_FAMILY_IGNORED;
			}
			physicalDevice = availablePhysicalDevices[deviceIndex];
			return VK_SUCCESS;
		}
		//�ú������ڴ����߼��豸����ȡ�ö���
		VkResult CreateDevice(VkDeviceCreateFlags flags = 0) 
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
			if (VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device)) {
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
			//������õ������豸����
			std::cout << "Renderer: " << physicalDeviceProperties.deviceName << std::endl;
			/*��Ch1-4���*/
			return VK_SUCCESS;
		}
		//���º������ڴ����߼��豸ʧ�ܺ�
		/*��鲢ȥ���������豸������չ������������չ����Ϊnullptr���������޸ĺ����չ��*/
		VkResult CheckDeviceExtensions(std::vector<const char*> extensionsToCheck, const char* layerName = nullptr) const {
		}
		//����ʵ��������չ
		void DeviceExtensions(const std::vector<const char*>& extensionNames) {
			deviceExtensions = extensionNames;
		}
		
		//��ȡ���õģ����棿����ʽ��������
		const VkFormat& AvailableSurfaceFormat(uint32_t index) const {
			return availableSurfaceFormats[index].format;
		}
		//��ȡ���õģ����棿����ɫ�ռ�
		const VkColorSpaceKHR& AvailableSurfaceColorSpace(uint32_t index) const {
			return availableSurfaceFormats[index].colorSpace;
		}
		//��ȡ���õģ����棿����ʽ��
		uint32_t AvailableSurfaceFormatCount() const {
			return uint32_t(availableSurfaceFormats.size());
		}

		//��ȡ������
		VkSwapchainKHR Swapchain() const {
			return swapchain;
		}
		//��ȡ��������ͼ��
		VkImage SwapchainImage(uint32_t index) const {
			return swapchainImages[index];
		}
		//��ȡ������ͼ���ʹ�÷�ʽ
		VkImageView SwapchainImageView(uint32_t index) const {
			return swapchainImageViews[index];
		}
		//��ȡͼ����
		uint32_t SwapchainImageCount() const {
			return uint32_t(swapchainImages.size());
		}
		//��ȡ������������Ϣ
		const VkSwapchainCreateInfoKHR& SwapchainCreateInfo() const {
			return swapchainCreateInfo;
		}

		//��ȡ���õģ����棿����ʽ
		VkResult GetSurfaceFormats() {
		}
		//���ÿ��õģ����棿����ʽ
		VkResult SetSurfaceFormat(VkSurfaceFormatKHR surfaceFormat) {
		}
		//�ú������ڴ���������
		VkResult CreateSwapchain(bool limitFrameRate = true, VkSwapchainCreateFlagsKHR flags = 0) {
		}
		//�ú��������ؽ�������
		VkResult RecreateSwapchain() {
		}

		//��ȡgraphicsBase����
		static graphicsBase& Base()
		{
			return singleton;
		}
	};
	//��̬������ʼ��
	graphicsBase graphicsBase::singleton;//
}