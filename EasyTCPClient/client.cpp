#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS

	#include <Windows.h>
	#include <WinSock2.h>

	#pragma comment(lib, "ws2_32.lib")
#else
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <string.h>

	#define SOKCET unsigned long long
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif

#include <iostream>
#include <thread>
#include "EasyTCPClient.hpp"

using namespace std;

bool g_bRun = true;
void cmdThread()
{
	while (true)
	{
		char cmdBuff[128]{};
		cin >> cmdBuff;
		if (!strcmp(cmdBuff, "exit")) {
			g_bRun = false;
			cout << "收到退出命令，结束任务！" << endl;
			return;
		}
		//else if (!strcmp(cmdBuff, "login")) {
		//	// 发送数据
		//	Login login;
		//	strcpy_s(login.username, "Peppa Pig");
		//	strcpy_s(login.password, "15213");
		//	client->SendData(&login);
		//}
		//else if (!strcmp(cmdBuff, "logout")) {
		//	// 发送数据
		//	Logout logout;
		//	strcpy_s(logout.username, "Peppa Pig");
		//	client->SendData(&logout);
		//}
		else {
			cout << "不支持的命令，请重新输入！" << endl;
		}
	}
}

int main()
{
	const int cCount = FD_SETSIZE - 1;
	EasyTCPClient* clients = new EasyTCPClient[cCount];
	for (int i = 0; i < cCount; i++) {
		clients[i].Connect("127.0.0.1", 6100);
	}

	// 启动UI线程
	thread t1(cmdThread);
	t1.detach();

	Login login;
	strcpy_s(login.username, "Peppa Pig");
	strcpy_s(login.password, "15213");
	
	while (g_bRun)
	{
		for (int i = 0; i < cCount; i++) {
			clients[i].SendData(&login);
			clients[i].OnRun();
		}
	}

	for (int i = 0; i < cCount; i++) {
		clients[i].Close();
	}
	delete[] clients;
	cout << "已退出！" << endl;

	Sleep(10000);
	return 0;
}
