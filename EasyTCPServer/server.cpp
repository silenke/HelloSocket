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

class MyServer : public EasyTCPServer
{
	// 被一个线程触发，安全
	void OnNetJoin(ClientSocket* pClient)
	{
		_clientCount++;
		std::cout << "client<" << pClient->sockfd() << ">join" << std::endl;
	}

	// 被多个线程触发，不安全
	void OnNetLeave(ClientSocket* pClient)
	{
		_clientCount--;
		std::cout << "client<" << pClient->sockfd() << ">leave" << std::endl;
	}

	// 被多个线程触发，不安全
	void OnNetMsg(ClientSocket* pClient, DataHeader* header)
	{
		_msgCount++;
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)header;
			//std::cout << "<socket=" << pClient->sockfd() << ">收到命令：CMD_LOGIN，"
			//	<< "数据长度：" << header->len << std::endl
			//	<< "username=" << login->username << " "
			//	<< "password=" << login->password << std::endl;

			// 发送数据
			LoginResult res;
			pClient->SendData(&res);
		}
		break;
		case CMD_LOGOUT:
		{
			Logout* logout = (Logout*)header;
			//std::cout << "<socket=" << pClient->sockfd() << ">收到命令：CMD_LOGIN，"
			//	<< "数据长度：" << header->len << std::endl
			//	<< "username=" << logout->username << std::endl;

			// 发送数据
			LogoutResult res;
			pClient->SendData(&res);
		}
		break;
		default:
		{
			std::cout << "<socket=" << pClient->sockfd() << ">收到未知命令，"
				<< "数据长度：" << header->len << std::endl;
			DataHeader header;
			pClient->SendData(&header);
		}
		break;
		}
	}

	// recv事件
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
