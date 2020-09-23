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
//			std::cout << "收到退出命令，结束任务！" << std::endl;
//			return;
//		}
//		else {
//			std::cout << "不支持的命令，请重新输入！" << std::endl;
//		}
//	}
//}


class MyServer : public EasyTCPServer
{
	// 被一个线程触发，安全
	void OnNetJoin(CellClient* pClient)
	{
		EasyTCPServer::OnNetJoin(pClient);
		//std::cout << "client<" << pClient->sockfd() << ">join" << std::endl;
	}

	// 被多个线程触发，不安全
	void OnNetLeave(CellClient* pClient)
	{
		EasyTCPServer::OnNetLeave(pClient);
		//std::cout << "client<" << pClient->sockfd() << ">leave" << std::endl;
	}

	// 被多个线程触发，不安全
	void OnNetMsg(CellServer* pCellServer, CellClient* pClient, netmsg_DataHeader* header)
	{
		EasyTCPServer::OnNetMsg(pCellServer, pClient, header);
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			netmsg_Login* login = (netmsg_Login*)header;
			//std::cout << "<socket=" << pClient->sockfd() << ">收到命令：CMD_LOGIN，"
			//	<< "数据长度：" << header->len << std::endl
			//	<< "username=" << login->username << " "
			//	<< "password=" << login->password << std::endl;

			pClient->resetDTHeart();

			// 发送数据
			//netmsg_LoginResult* pResult = new netmsg_LoginResult();
			//pCellServer->addSendTask(pClient, pResult);
			netmsg_LoginResult res;
			if (SOCKET_ERROR == pClient->SendData(&res))
			{
				// 发送缓冲区满，消息未发送
				CELLlog::Info("<Socket=%lld> Send Full\n", pClient->sockfd());
			}
		}
		break;
		case CMD_LOGOUT:
		{
			netmsg_Logout* logout = (netmsg_Logout*)header;
			//std::cout << "<socket=" << pClient->sockfd() << ">收到命令：CMD_LOGIN，"
			//	<< "数据长度：" << header->len << std::endl
			//	<< "username=" << logout->username << std::endl;

			// 发送数据
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
			CELLlog::Info("<Socket=%lld> 收到未知命令，数据长度：%d\n",
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

	// 启动UI线程
	//std::thread t1(cmdThread);
	//t1.detach();

	while (true)
	{
		char cmdBuff[128]{};
		std::cin >> cmdBuff;
		if (!strcmp(cmdBuff, "exit")) {
			server.Close();
			CELLlog::Info("收到退出命令，结束任务！\n");
			break;
		}
		else {
			CELLlog::Info("不支持的命令，请重新输入！\n");
		}
	}

	CELLlog::Info("已退出！\n");

//#ifdef _WIN32
//	while (true) Sleep(1);
//#endif // _WIN32

	return 0;
}
