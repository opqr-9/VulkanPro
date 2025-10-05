#include "VKBase.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#pragma comment(lib, "glfw3.lib")	//链接编译所需的静态库

GLFWwindow* pWindow;				//窗口指针
GLFWmonitor* pMonitor;				//显示器信息的指针
const char* windowTitle = "EasyVK";	//窗口标题

using namespace vulkan;

//初始化窗口，成功返回true，失败返回false
//参数：size：窗口大小；fullScreen：是否以全屏初始化窗口；isResizable：是否可拉伸窗口；limitFrameRate：是否将帧数限制到不超过屏幕刷新率
bool InitializeWindow (VkExtent2D size, bool fullScreen = false, bool isResizable = true, bool limitFrameRate = true)
{
	//判断glfw是否初始化
	if (!glfwInit())
	{
		std::cout << "[ InitializeWindow ] ERROR\nFailed to initialize GLFW!\n";
		return false;
	}

	//因为GLFW_CLIENT_API默认为GLFW_OPENGL_API，会创建OpenGL的上下文，这对于Vulkan是多余的，所以利用GLFW_NO_API向GLFW说明不需要OpenGL的API
	//原理大概是让GLFW_CLIENT_API变为特殊值（即GLFW_NO_API）？
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);	//向GLFW说明不需要OpenGL的API
	glfwWindowHint(GLFW_RESIZABLE, isResizable);	//指定窗口是否可伸缩

	pMonitor = glfwGetPrimaryMonitor();				//取得当前显示器的指针，以便进行全屏或其他操作

	//用显示器的指针获取显示器当前的视频模式，保证全屏时的图像区域与屏幕分辨率一致
	//不设置全局变量是因为在程序运行期间可能会发生视频模式的变更
	//tips：pMode指向的数组不需要手动释放，它由GLFW分配和释放
	const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);	//显示器视频模式指针

	//创建窗口,利用fullScreen决定是否全屏,创建失败返回nullptr
	//前三个参数分别代表窗口的宽，高，标题，第四个参数用于指定全屏模式的显示器，如果为nullptr则为窗口模式，第五个参数可传入一个其他窗口的指针，用于与其他窗口分享内容
	pWindow = fullScreen ? glfwCreateWindow(pMode->width, pMode->height, windowTitle, pMonitor, nullptr):
						   glfwCreateWindow(size.width, size.height, windowTitle, nullptr, nullptr);
	//判断窗口是否创建失败
	if (!pWindow)
	{
		std::cout << "[ InitializeWindow ]\nFailed to create a glfw window!\n";
		glfwTerminate();	//终止glfw库
		return false;
	}

#ifdef _WIN32
	graphicsBase::Base().AddInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME);
	graphicsBase::Base().AddInstanceExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
	uint32_t extensionCount = 0;	//扩展数
	const char** extensionNames;	//扩展名数组,由glfw控制分配释放
	extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);//获取所需扩展，同时若执行成功则将扩展数写入传入参数
	if (!extensionNames)	//查看扩展名数组是否为空
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
	//在创建window surface前创建Vulkan实例
	graphicsBase::Base().UseLatestApiVersion();
	if (graphicsBase::Base().CreateInstance())
		return false;

	//创建window surface
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	if (VkResult result = glfwCreateWindowSurface(graphicsBase::Base().Instance(), pWindow, nullptr, &surface)) {
		std::cout << "[ InitializeWindow ] ERROR\nFailed to create a window surface!\nError code: " << int32_t(result) << std::endl;
		glfwTerminate();
		return false;
	}
	graphicsBase::Base().Surface(surface);
	//通过用||操作符短路执行来省去几行
	if (//获取物理设备，并使用列表中的第一个物理设备，这里不考虑以下任意函数失败后更换物理设备的情况
		graphicsBase::Base().GetPhysicalDevices() ||
		//一个true一个false，暂时不需要计算用的队列
		graphicsBase::Base().DeterminePhysicalDevice(0, true, false) ||
		//创建逻辑设备
		graphicsBase::Base().CreateDevice())
		return false;
	return true;
}

//终止窗口
void TerminateWindow() 
{

}

//在窗口标题上显示帧率
//原理：记录t0，若某次调用的时间t1与t0大于1s，将这中间的帧数（dframe）除以时间差值（dt）即可得到帧率，在之后将t1赋值给t0，dframe变为0，通过字符串操作将帧率衔接在标题后，再使用glfwSetWindowTitle(...)设置标题
void TitleFps()
{
	//glfwGetTime()返回自GLFW初始化以来的时间，单位为秒
	static double time0 = glfwGetTime();//
	static double time1;
	static double dt;
	static int dframe=-1;
	//TODO: std::stringstream是什么//////////////////////////////////////////////////
	static std::stringstream info;//
	time1 = glfwGetTime();
	dframe++;
	if ((dt = time1 - time0) >= 1)
	{
		//TODO: precision作用//////////////////////////////////////////////////
		info.precision(1);
		//TODO: std::fixed是什么//////////////////////////////////////////////////
		info << windowTitle << " " << std::fixed << dframe / dt << " FPS";	//将windowTitle+“ ”+dframe/dt+“ FPS”加入缓冲流
		glfwSetWindowTitle(pWindow, info.str().c_str());					//设置窗口标题
		info.str("");														//清空stringstream
		time0 = time1;
		dframe = 0;
	}
}

//glfwSetWindowMonitor(window, monitor, xpos, ypos, width, height, refreshRate);
//window为窗口的指针，monitor为显示器信息的指针，xpos和ypos分别为图像区域左上角的横坐标和纵坐标，width和height分别为图像区域的宽和高，refreshRate参数为屏幕刷新率
//tips：1.图像区域不包括窗口边框和标题栏。2.切换到全屏模式时，xpos和ypos将被忽略。3.切换到窗口模式,ypos应避免小于标题栏高度
/*窗口变全屏*/
void MakeWindowFullScreen()
{
	const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);
	glfwSetWindowMonitor(pWindow, pMonitor, 0, 0, pMode->width, pMode->height,pMode->refreshRate);
}
//全屏变窗口
void MakeWindowWindowed(VkOffset2D position, VkExtent2D size)
{
	const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);
	glfwSetWindowMonitor(pWindow, nullptr, position.x, position.y, size.width, size.height, pMode->refreshRate);
}