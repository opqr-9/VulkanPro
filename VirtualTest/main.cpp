#include "GlfwGeneral.hpp"

int main() {
	if (!InitializeWindow({ 1280,720 }))
	{
		return -1;
	}
	//glfwWindowShouldClose(pWindow)������Ϊtrue˵����ǰһ�����յ������Բ���ϵͳ����Ϣ������GLFWӦ���رմ��ڡ�
	while (!glfwWindowShouldClose(pWindow))
	{
		//glfwPollEvents()���ڴ���GLFW��ص��¼�������մ��ڱ仯����Ϣ�����������벢ִ�лص�������
		glfwPollEvents();
		TitleFps();
	}
	TerminateWindow(); 
	return 0;
}