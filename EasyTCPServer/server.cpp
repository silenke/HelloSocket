#include <iostream>
#include <thread>
#include "EasyTCPServer.hpp"


//bool g_bRun = true;
//void cmdThread()
//{
//	while (true)
//	{
//		char cmdBuff[128]{};
//		std::cin >> cmdBuff;
//		if (!strcmp(cmdBuff, "exit")) {
//			g_bRun = false;
//			std::cout << "�յ��˳������������" << std::endl;
//			return;
//		}
//		else {
//			std::cout << "��֧�ֵ�������������룡" << std::endl;
//		}
//	}
//}


class MyServer : public EasyTCPServer
{
	// ��һ���̴߳�������ȫ
	void OnNetJoin(CellClient* pClient)
	{
		EasyTCPServer::OnNetJoin(pClient);
		//std::cout << "client<" << pClient->sockfd() << ">join" << std::endl;
	}

	// ������̴߳���������ȫ
	void OnNetLeave(CellClient* pClient)
	{
		EasyTCPServer::OnNetLeave(pClient);
		//std::cout << "client<" << pClient->sockfd() << ">leave" << std::endl;
	}

	// ������̴߳���������ȫ
	void OnNetMsg(CellServer* pCellServer, CellClient* pClient, netmsg_DataHeader* header)
	{
		EasyTCPServer::OnNetMsg(pCellServer, pClient, header);
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			netmsg_Login* login = (netmsg_Login*)header;
			//std::cout << "<socket=" << pClient->sockfd() << ">�յ����CMD_LOGIN��"
			//	<< "���ݳ��ȣ�" << header->len << std::endl
			//	<< "username=" << login->username << " "
			//	<< "password=" << login->password << std::endl;

			pClient->resetDTHeart();

			// ��������
			//netmsg_LoginResult* pResult = new netmsg_LoginResult();
			//pCellServer->addSendTask(pClient, pResult);
			netmsg_LoginResult res;
			if (SOCKET_ERROR == pClient->SendData(&res))
			{
				// ���ͻ�����������Ϣδ����
				CELLlog::Info("<Socket=%lld> Send Full\n", pClient->sockfd());
			}
		}
		break;
		case CMD_LOGOUT:
		{
			netmsg_Logout* logout = (netmsg_Logout*)header;
			//std::cout << "<socket=" << pClient->sockfd() << ">�յ����CMD_LOGIN��"
			//	<< "���ݳ��ȣ�" << header->len << std::endl
			//	<< "username=" << logout->username << std::endl;

			// ��������
			netmsg_LogoutResult* pResult = new netmsg_LogoutResult();
			pCellServer->addSendTask(pClient, pResult);
		}
		break;
		case CMD_HEART_C2S:
		{
			pClient->resetDTHeart();
			netmsg_Heart_S2C* pHeart = new netmsg_Heart_S2C();
			pCellServer->addSendTask(pClient, pHeart);
		}
		break;
		default:
		{
			CELLlog::Info("<Socket=%lld> �յ�δ֪������ݳ��ȣ�%d\n",
				pClient->sockfd(), header->len);
			netmsg_DataHeader* pResult = new netmsg_DataHeader();
			pCellServer->addSendTask(pClient, pResult);
		}
		break;
		}
	}
};


int main()
{
	CELLlog::Instance().setLogPath("serverLog.txt", "w");

	MyServer server;
	server.InitSocket();
	server.Bind(nullptr, 6100);
	server.Listen(5);
	server.Start(4);

	// ����UI�߳�
	//std::thread t1(cmdThread);
	//t1.detach();

	while (true)
	{
		char cmdBuff[128]{};
		std::cin >> cmdBuff;
		if (!strcmp(cmdBuff, "exit")) {
			server.Close();
			CELLlog::Info("�յ��˳������������\n");
			break;
		}
		else {
			CELLlog::Info("��֧�ֵ�������������룡\n");
		}
	}

	CELLlog::Info("���˳���\n");

//#ifdef _WIN32
//	while (true) Sleep(1);
//#endif // _WIN32

	return 0;
}
