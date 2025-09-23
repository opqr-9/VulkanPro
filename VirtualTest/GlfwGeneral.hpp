#include "VKBase.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#pragma comment(lib, "glfw3.lib") //链接编译所需的静态库

//窗口指针
GLFWwindow* pWindow;

//显示器信息的指针
GLFWmonitor* pMonitor;

//窗口标题
const char* windowTitle = "EasyVK";

//初始化窗口，成功返回true，失败返回false
//参数：size：窗口大小；fullScreen：是否以全屏初始化窗口；isResizable：是否可拉伸窗口；limitFrameRate：是否将帧数限制到不超过屏幕刷新率
bool InitializeWindow (VkExtent2D size, bool fullScreen = false, bool isResizable = true, bool limitFrameRate = true)
{
	if (!glfwInit())
	{
		std::cout << "[ InitializeWindow ] ERROR\nFailed to initialize GLFW!\n";
		return false;
	}

	//因为GLFW_CLIENT_API默认为GLFW_OPENGL_API，会创建OpenGL的上下文，这对于Vulkan是多余的，所以利用GLFW_NO_API向GLFW说明不需要OpenGL的API
	//原理大概是让GLFW_CLIENT_API变为特殊值（即GLFW_NO_API）？
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	//指定窗口是否可伸缩
	glfwWindowHint(GLFW_RESIZABLE, isResizable);

	//取得当前显示器的指针，以便进行全屏或其他操作
	pMonitor = glfwGetPrimaryMonitor();

	//用显示器的指针获取显示器当前的视频模式，保证全屏时的图像区域与屏幕分辨率一致
	//不设置全局变量是因为在程序运行期间可能会发生视频模式的变更
	//tips：pMode指向的数组不需要手动释放，它由GLFW分配和释放
	const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);

	//创建窗口,利用fullScreen决定是否全屏,创建失败返回nullptr
	//前三个参数分别代表窗口的宽，高，标题，第四个参数用于指定全屏模式的显示器，如果为nullptr则为窗口模式，第五个参数可传入一个其他窗口的指针，用于与其他窗口分享内容
	pWindow = fullScreen ? glfwCreateWindow(pMode->width, pMode->height, windowTitle, pMonitor, nullptr):
						   glfwCreateWindow(size.width, size.height, windowTitle, nullptr, nullptr);

	//判断窗口是否创建失败
	if (!pWindow)
	{
		std::cout << "[ InitializeWindow ]\nFailed to create a glfw window!\n";
		//终止glfw库
		glfwTerminate();
		return false;
	}

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
	static double time0 = glfwGetTime();
	static double time1;
	static double dt;
	static int dframe=-1;
	//TODO: std::stringstream是什么//////////////////////////////////////////////////
	static std::stringstream info;
	time1 = glfwGetTime();
	dframe++;
	if ((dt = time1 - time0) >= 1)
	{
		//TODO: precision作用//////////////////////////////////////////////////
		info.precision(1);
		//将windowTitle+“ ”+dframe/dt+“ FPS”加入缓冲流
		//TODO: std::fixed是什么//////////////////////////////////////////////////
		info << windowTitle << " " << std::fixed << dframe / dt << " FPS";
		//设置窗口标题
		glfwSetWindowTitle(pWindow, info.str().c_str());
		info.str("");
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