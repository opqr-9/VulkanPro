#include "VKBase.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#pragma comment(lib, "glfw3.lib") //���ӱ�������ľ�̬��

//����ָ��
GLFWwindow* pWindow;

//��ʾ����Ϣ��ָ��
GLFWmonitor* pMonitor;

//���ڱ���
const char* windowTitle = "EasyVK";

//��ʼ�����ڣ��ɹ�����true��ʧ�ܷ���false
//������size�����ڴ�С��fullScreen���Ƿ���ȫ����ʼ�����ڣ�isResizable���Ƿ�����촰�ڣ�limitFrameRate���Ƿ�֡�����Ƶ���������Ļˢ����
bool InitializeWindow (VkExtent2D size, bool fullScreen = false, bool isResizable = true, bool limitFrameRate = true)
{
	if (!glfwInit())
	{
		std::cout << "[ InitializeWindow ] ERROR\nFailed to initialize GLFW!\n";
		return false;
	}

	//��ΪGLFW_CLIENT_APIĬ��ΪGLFW_OPENGL_API���ᴴ��OpenGL�������ģ������Vulkan�Ƕ���ģ���������GLFW_NO_API��GLFW˵������ҪOpenGL��API
	//ԭ��������GLFW_CLIENT_API��Ϊ����ֵ����GLFW_NO_API����
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	//ָ�������Ƿ������
	glfwWindowHint(GLFW_RESIZABLE, isResizable);

	//ȡ�õ�ǰ��ʾ����ָ�룬�Ա����ȫ������������
	pMonitor = glfwGetPrimaryMonitor();

	//����ʾ����ָ���ȡ��ʾ����ǰ����Ƶģʽ����֤ȫ��ʱ��ͼ����������Ļ�ֱ���һ��
	//������ȫ�ֱ�������Ϊ�ڳ��������ڼ���ܻᷢ����Ƶģʽ�ı��
	//tips��pModeָ������鲻��Ҫ�ֶ��ͷţ�����GLFW������ͷ�
	const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);

	//��������,����fullScreen�����Ƿ�ȫ��,����ʧ�ܷ���nullptr
	//ǰ���������ֱ�����ڵĿ��ߣ����⣬���ĸ���������ָ��ȫ��ģʽ����ʾ�������Ϊnullptr��Ϊ����ģʽ������������ɴ���һ���������ڵ�ָ�룬�������������ڷ�������
	pWindow = fullScreen ? glfwCreateWindow(pMode->width, pMode->height, windowTitle, pMonitor, nullptr):
						   glfwCreateWindow(size.width, size.height, windowTitle, nullptr, nullptr);

	//�жϴ����Ƿ񴴽�ʧ��
	if (!pWindow)
	{
		std::cout << "[ InitializeWindow ]\nFailed to create a glfw window!\n";
		//��ֹglfw��
		glfwTerminate();
		return false;
	}

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
	static double time0 = glfwGetTime();
	static double time1;
	static double dt;
	static int dframe=-1;
	//TODO: std::stringstream��ʲô//////////////////////////////////////////////////
	static std::stringstream info;
	time1 = glfwGetTime();
	dframe++;
	if ((dt = time1 - time0) >= 1)
	{
		//TODO: precision����//////////////////////////////////////////////////
		info.precision(1);
		//��windowTitle+�� ��+dframe/dt+�� FPS�����뻺����
		//TODO: std::fixed��ʲô//////////////////////////////////////////////////
		info << windowTitle << " " << std::fixed << dframe / dt << " FPS";
		//���ô��ڱ���
		glfwSetWindowTitle(pWindow, info.str().c_str());
		info.str("");
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