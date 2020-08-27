#include <iostream>
#include <thread>
#include "EasyTCPServer.hpp"

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
			cout << "�յ��˳������������" << endl;
			return;
		}
		else {
			cout << "��֧�ֵ�������������룡" << endl;
		}
	}
}

int main()
{
	EasyTCPServer server;
	server.InitSocket();
	server.Bind(nullptr, 6100);
	server.Listen(5);

	// ����UI�߳�
	thread t1(cmdThread);
	t1.detach();

	while (g_bRun)
	{
		server.OnRun();
	}

	server.Close();
	cout << "���˳���" << endl;

	Sleep(10000);
	return 0;
}
