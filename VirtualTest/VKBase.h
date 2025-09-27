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
		VkResult GetQueueFamilyIndices(VkPhysicalDevice physicalDevice, bool enableGraphicsQueue, bool enableComputeQueue, uint32_t(&queueFamilyIndices)[3]) {
			
		}

		std::vector <VkSurfaceFormatKHR> availableSurfaceFormats;	//���õģ����棿����ʽ��

		VkSwapchainKHR swapchain;									//������
		std::vector <VkImage> swapchainImages;						//����ͼ����豸�ڴ���
		std::vector <VkImageView> swapchainImageViews;				//ͼ���ʹ�÷�ʽ��
		//���潻�����Ĵ�����Ϣ�Ա��ؽ�������							
		VkSwapchainCreateInfoKHR swapchainCreateInfo = {};			//������������Ϣ

		static graphicsBase singleton;								//graphicsBase����

		//��instanceLayers��instanceExtensions����������ַ���ָ�룬ȷ�����ظ�
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

		//����DebugMessenger
		VkResult CreateDebugMessenger() 
		{

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
			//������Vulkanʵ��������Ŵ���debug messenger
			CreateDebugMessenger();
		#endif
			return VK_SUCCESS;
		}
		//���º������ڴ���Vulkanʵ��ʧ�ܺ�
		/*��鲢ȥ��������ʵ������㼶���������ò㼶����Ϊnullptr���������޸ĺ�Ĳ㼶��*/
		VkResult CheckInstanceLayers(std::vector<const char*> layersToCheck)
		{

		}
		//����ʵ������㼶
		void InstanceLayers(const std::vector<const char*>& layerNames) 
		{
			instanceLayers = layerNames;
		}
		//��鲢ȥ��������ʵ��������չ������������չ����Ϊnullptr���������޸ĺ����չ��
		VkResult CheckInstanceExtensions(std::vector<const char*> extensionsToCheck, const char* layerName = nullptr) const 
		{

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
		VkResult GetPhysicalDevices() {
		}
		//�ú�������ָ�����������豸������GetQueueFamilyIndices(...)ȡ�ö���������
		VkResult DeterminePhysicalDevice(uint32_t deviceIndex = 0, bool enableGraphicsQueue = true, bool enableComputeQueue = true) {
		}
		//�ú������ڴ����߼��豸����ȡ�ö���
		VkResult CreateDevice(VkDeviceCreateFlags flags = 0) {
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