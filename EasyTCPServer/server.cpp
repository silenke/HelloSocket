#include <iostream>
#include <thread>
#include "EasyTCPServer.hpp"

bool g_bRun = true;
void cmdThread()
{
	while (true)
	{
		char cmdBuff[128]{};
		std::cin >> cmdBuff;
		if (!strcmp(cmdBuff, "exit")) {
			g_bRun = false;
			std::cout << "�յ��˳������������" << std::endl;
			return;
		}
		else {
			std::cout << "��֧�ֵ�������������룡" << std::endl;
		}
	}
}

int main()
{
	EasyTCPServer server;
	server.InitSocket();
	server.Bind(nullptr, 6100);
	server.Listen(5);
	server.Start();

	// ����UI�߳�
	std::thread t1(cmdThread);
	t1.detach();

	while (g_bRun)
	{
		server.OnRun();
	}

	server.Close();
	std::cout << "���˳���" << std::endl;

	Sleep(10000);
	return 0;
}
