#include "GlfwGeneral.hpp"

int main() {
	if (!InitializeWindow({ 1280,720 }))
	{
		return -1;
	}
	//glfwWindowShouldClose(pWindow)若返回为true说明在前一次中收到了来自操作系统的信息，告诉GLFW应当关闭窗口。
	while (!glfwWindowShouldClose(pWindow))
	{
		//glfwPollEvents()用于处理GLFW相关的事件，如接收窗口变化的信息，及接受输入并执行回调函数。
		glfwPollEvents();
		TitleFps();
	}
	TerminateWindow(); 
	return 0;
}