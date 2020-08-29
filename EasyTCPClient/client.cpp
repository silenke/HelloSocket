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

const int cCount = 1000;
const int tCount = 4;
EasyTCPClient* clients[cCount];

void sendThread(int id)
{
	int c = cCount / tCount;
	int begin = id * c;
	int end = id * c + c;
	for (int i = begin; i < end; i++)
	{
		if (!g_bRun)
		{
			for (int j = begin; j < i; j++)
			{
				clients[j]->Close();
				delete clients[j];
			}
			return;
		}
		clients[i] = new EasyTCPClient();
		clients[i]->Connect("127.0.0.1", 6100);
		cout << "Connect=" << i << endl;
	}

	Login login;
	strcpy_s(login.username, "Peppa Pig");
	strcpy_s(login.password, "15213");

	while (g_bRun)
	{
		for (int i = begin; i < end; i++)
		{
			clients[i]->SendData(&login);
			clients[i]->OnRun();
		}
	}

	for (int i = begin; i < end; i++)
	{
		clients[i]->Close();
		delete clients[i];
	}
}

int main()
{
	// 启动UI线程
	thread t1(cmdThread);
	t1.detach();

	// 启动发送线程
	for (int i = 0; i < tCount; i++)
	{
		thread t1(sendThread, i);
		t1.detach();
	}

	cout << "已退出！" << endl;

	Sleep(1000000);
	return 0;
}
