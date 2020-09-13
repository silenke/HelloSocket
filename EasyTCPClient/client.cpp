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
#include "CELLTimestamp.hpp"


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
		//else if (!strcmp(cmdBuff, "login")) {
		//	// ��������
		//	netmsg_Login login;
		//	strcpy_s(login.username, "Peppa Pig");
		//	strcpy_s(login.password, "15213");
		//	client->SendData(&login);
		//}
		//else if (!strcmp(cmdBuff, "logout")) {
		//	// ��������
		//	netmsg_Logout logout;
		//	strcpy_s(logout.username, "Peppa Pig");
		//	client->SendData(&logout);
		//}
		else {
			std::cout << "��֧�ֵ�������������룡" << std::endl;
		}
	}
}


const int cCount = 100;
const int tCount = 4;
EasyTCPClient* clients[cCount];
std::atomic_int sendCount = 0;
std::atomic_int readCount = 0;


void recvThread(int begin, int end)
{
	while (g_bRun)
	{
		for (int i = begin; i < end; i++)
		{
			clients[i]->OnRun();
		}
	}
}


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
		//std::cout << "thread<" << id << ">��Connect=" << i << std::endl;
	}

	std::cout << "thread<" << id
		<< ">��Connect<begin=" << begin << "��end=" << end << ">" << std::endl;

	readCount++;
	while (readCount < tCount)	// �ȴ������߳�
	{
		std::chrono::milliseconds t(10);
		std::this_thread::sleep_for(t);
	}

	std::thread t1(recvThread, begin, end);
	t1.detach();

	netmsg_Login login;
	strcpy_s(login.username, "Peppa Pig");
	strcpy_s(login.password, "15213");

	while (g_bRun)
	{
		for (int i = begin; i < end; i++)
		{
			if (SOCKET_ERROR != clients[i]->SendData(&login))
			{
				sendCount++;
			}
		}
		//std::chrono::milliseconds t(10);
		//std::this_thread::sleep_for(t);
	}

	for (int i = begin; i < end; i++)
	{
		clients[i]->Close();
		delete clients[i];
	}
}


int main()
{
	// ����UI�߳�
	std::thread t1(cmdThread);
	t1.detach();

	// ���������߳�
	for (int i = 0; i < tCount; i++)
	{
		std::thread t1(sendThread, i);
		t1.detach();
	}

	CELLTimestamp tTime;
	while (g_bRun)
	{
		auto t = tTime.getElapsedSecond();
		if (t >= 1.0)
		{
			std::cout << "thread<" << tCount
				<< ">��time<" << t
				<< ">��clients<" << cCount
				<< ">��send<" << sendCount
				<< ">" << std::endl;
			sendCount = 0;
			tTime.update();
		}
		Sleep(1);
	}

	std::cout << "���˳���" << std::endl;

	return 0;
}
