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

class MyServer : public EasyTCPServer
{
	// ��һ���̴߳�������ȫ
	void OnNetJoin(ClientSocket* pClient)
	{
		_clientCount++;
		std::cout << "client<" << pClient->sockfd() << ">join" << std::endl;
	}

	// ������̴߳���������ȫ
	void OnNetLeave(ClientSocket* pClient)
	{
		_clientCount--;
		std::cout << "client<" << pClient->sockfd() << ">leave" << std::endl;
	}

	// ������̴߳���������ȫ
	void OnNetMsg(ClientSocket* pClient, DataHeader* header)
	{
		_msgCount++;
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)header;
			//std::cout << "<socket=" << pClient->sockfd() << ">�յ����CMD_LOGIN��"
			//	<< "���ݳ��ȣ�" << header->len << std::endl
			//	<< "username=" << login->username << " "
			//	<< "password=" << login->password << std::endl;

			// ��������
			LoginResult res;
			pClient->SendData(&res);
		}
		break;
		case CMD_LOGOUT:
		{
			Logout* logout = (Logout*)header;
			//std::cout << "<socket=" << pClient->sockfd() << ">�յ����CMD_LOGIN��"
			//	<< "���ݳ��ȣ�" << header->len << std::endl
			//	<< "username=" << logout->username << std::endl;

			// ��������
			LogoutResult res;
			pClient->SendData(&res);
		}
		break;
		default:
		{
			std::cout << "<socket=" << pClient->sockfd() << ">�յ�δ֪���"
				<< "���ݳ��ȣ�" << header->len << std::endl;
			DataHeader header;
			pClient->SendData(&header);
		}
		break;
		}
	}

	// recv�¼�
	void OnNetRecv(ClientSocket* pClient)
	{
		_recvCount++;
	}
};

int main()
{
	MyServer server;
	server.InitSocket();
	server.Bind(nullptr, 6100);
	server.Listen(5);
	server.Start(4);

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
