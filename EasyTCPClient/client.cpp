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

void cmdThread(EasyTCPClient* client)
{
	while (true)
	{
		char cmdBuff[128]{};
		cin >> cmdBuff;
		if (!strcmp(cmdBuff, "exit")) {
			client->Close();
			cout << "收到退出命令，结束任务！" << endl;
			return;
		}
		else if (!strcmp(cmdBuff, "login")) {
			// 发送数据
			Login login;
			strcpy_s(login.username, "Peppa Pig");
			strcpy_s(login.password, "15213");
			client->SendData(&login);
		}
		else if (!strcmp(cmdBuff, "logout")) {
			// 发送数据
			Logout logout;
			strcpy_s(logout.username, "Peppa Pig");
			client->SendData(&logout);
		}
		else {
			cout << "不支持的命令，请重新输入！" << endl;
		}
	}
}

int main()
{
	EasyTCPClient client;
	client.Connect("127.0.0.1", 6100);

	EasyTCPClient client2;
	client2.Connect("127.0.0.1", 6200);

	// 启动UI线程
	thread t1(cmdThread, &client);
	t1.detach();

	thread t2(cmdThread, &client2);
	t2.detach();

	Login login;
	strcpy_s(login.username, "Peppa Pig");
	strcpy_s(login.password, "15213");

	while (client.isRun() || client2.isRun())
	{
		client.OnRun();
		client2.OnRun();
	}

	client.Close();
	client2.Close();
	cout << "已退出！" << endl;

	Sleep(10000);
	return 0;
}
