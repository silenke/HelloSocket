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
			std::cout << "收到退出命令，结束任务！" << std::endl;
			return;
		}
		else {
			std::cout << "不支持的命令，请重新输入！" << std::endl;
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

	// 启动UI线程
	std::thread t1(cmdThread);
	t1.detach();

	while (g_bRun)
	{
		server.OnRun();
	}

	server.Close();
	std::cout << "已退出！" << std::endl;

	Sleep(10000);
	return 0;
}
