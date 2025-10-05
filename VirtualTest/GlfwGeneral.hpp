#include "VKBase.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#pragma comment(lib, "glfw3.lib")	//���ӱ�������ľ�̬��

GLFWwindow* pWindow;				//����ָ��
GLFWmonitor* pMonitor;				//��ʾ����Ϣ��ָ��
const char* windowTitle = "EasyVK";	//���ڱ���

using namespace vulkan;

//��ʼ�����ڣ��ɹ�����true��ʧ�ܷ���false
//������size�����ڴ�С��fullScreen���Ƿ���ȫ����ʼ�����ڣ�isResizable���Ƿ�����촰�ڣ�limitFrameRate���Ƿ�֡�����Ƶ���������Ļˢ����
bool InitializeWindow (VkExtent2D size, bool fullScreen = false, bool isResizable = true, bool limitFrameRate = true)
{
	//�ж�glfw�Ƿ��ʼ��
	if (!glfwInit())
	{
		std::cout << "[ InitializeWindow ] ERROR\nFailed to initialize GLFW!\n";
		return false;
	}

	//��ΪGLFW_CLIENT_APIĬ��ΪGLFW_OPENGL_API���ᴴ��OpenGL�������ģ������Vulkan�Ƕ���ģ���������GLFW_NO_API��GLFW˵������ҪOpenGL��API
	//ԭ��������GLFW_CLIENT_API��Ϊ����ֵ����GLFW_NO_API����
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);	//��GLFW˵������ҪOpenGL��API
	glfwWindowHint(GLFW_RESIZABLE, isResizable);	//ָ�������Ƿ������

	pMonitor = glfwGetPrimaryMonitor();				//ȡ�õ�ǰ��ʾ����ָ�룬�Ա����ȫ������������

	//����ʾ����ָ���ȡ��ʾ����ǰ����Ƶģʽ����֤ȫ��ʱ��ͼ����������Ļ�ֱ���һ��
	//������ȫ�ֱ�������Ϊ�ڳ��������ڼ���ܻᷢ����Ƶģʽ�ı��
	//tips��pModeָ������鲻��Ҫ�ֶ��ͷţ�����GLFW������ͷ�
	const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);	//��ʾ����Ƶģʽָ��

	//��������,����fullScreen�����Ƿ�ȫ��,����ʧ�ܷ���nullptr
	//ǰ���������ֱ�����ڵĿ��ߣ����⣬���ĸ���������ָ��ȫ��ģʽ����ʾ�������Ϊnullptr��Ϊ����ģʽ������������ɴ���һ���������ڵ�ָ�룬�������������ڷ�������
	pWindow = fullScreen ? glfwCreateWindow(pMode->width, pMode->height, windowTitle, pMonitor, nullptr):
						   glfwCreateWindow(size.width, size.height, windowTitle, nullptr, nullptr);
	//�жϴ����Ƿ񴴽�ʧ��
	if (!pWindow)
	{
		std::cout << "[ InitializeWindow ]\nFailed to create a glfw window!\n";
		glfwTerminate();	//��ֹglfw��
		return false;
	}

#ifdef _WIN32
	graphicsBase::Base().AddInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME);
	graphicsBase::Base().AddInstanceExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
	uint32_t extensionCount = 0;	//��չ��
	const char** extensionNames;	//��չ������,��glfw���Ʒ����ͷ�
	extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);//��ȡ������չ��ͬʱ��ִ�гɹ�����չ��д�봫�����
	if (!extensionNames)	//�鿴��չ�������Ƿ�Ϊ��
	{
		std::cout << "[ InitializeWindow ]\nVulkan is not available on this machine!\n";
		glfwTerminate();
		return false;
	}
	for (size_t i = 0; i < extensionCount; i++)
	{
		graphicsBase::Base().AddInstanceExtension(extensionNames[i]);
	}
#endif
	graphicsBase::Base().AddDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	//�ڴ���window surfaceǰ����Vulkanʵ��
	graphicsBase::Base().UseLatestApiVersion();
	if (graphicsBase::Base().CreateInstance())
		return false;

	//����window surface
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	if (VkResult result = glfwCreateWindowSurface(graphicsBase::Base().Instance(), pWindow, nullptr, &surface)) {
		std::cout << "[ InitializeWindow ] ERROR\nFailed to create a window surface!\nError code: " << int32_t(result) << std::endl;
		glfwTerminate();
		return false;
	}
	graphicsBase::Base().Surface(surface);
	//ͨ����||��������·ִ����ʡȥ����
	if (//��ȡ�����豸����ʹ���б��еĵ�һ�������豸�����ﲻ�����������⺯��ʧ�ܺ���������豸�����
		graphicsBase::Base().GetPhysicalDevices() ||
		//һ��trueһ��false����ʱ����Ҫ�����õĶ���
		graphicsBase::Base().DeterminePhysicalDevice(0, true, false) ||
		//�����߼��豸
		graphicsBase::Base().CreateDevice())
		return false;
	return true;
}

//��ֹ����
void TerminateWindow() 
{

}

//�ڴ��ڱ�������ʾ֡��
//ԭ����¼t0����ĳ�ε��õ�ʱ��t1��t0����1s�������м��֡����dframe������ʱ���ֵ��dt�����ɵõ�֡�ʣ���֮��t1��ֵ��t0��dframe��Ϊ0��ͨ���ַ���������֡���ν��ڱ������ʹ��glfwSetWindowTitle(...)���ñ���
void TitleFps()
{
	//glfwGetTime()������GLFW��ʼ��������ʱ�䣬��λΪ��
	static double time0 = glfwGetTime();//
	static double time1;
	static double dt;
	static int dframe=-1;
	//TODO: std::stringstream��ʲô//////////////////////////////////////////////////
	static std::stringstream info;//
	time1 = glfwGetTime();
	dframe++;
	if ((dt = time1 - time0) >= 1)
	{
		//TODO: precision����//////////////////////////////////////////////////
		info.precision(1);
		//TODO: std::fixed��ʲô//////////////////////////////////////////////////
		info << windowTitle << " " << std::fixed << dframe / dt << " FPS";	//��windowTitle+�� ��+dframe/dt+�� FPS�����뻺����
		glfwSetWindowTitle(pWindow, info.str().c_str());					//���ô��ڱ���
		info.str("");														//���stringstream
		time0 = time1;
		dframe = 0;
	}
}

//glfwSetWindowMonitor(window, monitor, xpos, ypos, width, height, refreshRate);
//windowΪ���ڵ�ָ�룬monitorΪ��ʾ����Ϣ��ָ�룬xpos��ypos�ֱ�Ϊͼ���������Ͻǵĺ�����������꣬width��height�ֱ�Ϊͼ������Ŀ�͸ߣ�refreshRate����Ϊ��Ļˢ����
//tips��1.ͼ�����򲻰������ڱ߿�ͱ�������2.�л���ȫ��ģʽʱ��xpos��ypos�������ԡ�3.�л�������ģʽ,yposӦ����С�ڱ������߶�
/*���ڱ�ȫ��*/
void MakeWindowFullScreen()
{
	const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);
	glfwSetWindowMonitor(pWindow, pMonitor, 0, 0, pMode->width, pMode->height,pMode->refreshRate);
}
//ȫ���䴰��
void MakeWindowWindowed(VkOffset2D position, VkExtent2D size)
{
	const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);
	glfwSetWindowMonitor(pWindow, nullptr, position.x, position.y, size.width, size.height, pMode->refreshRate);
}